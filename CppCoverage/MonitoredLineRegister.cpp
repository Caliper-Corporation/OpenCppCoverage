// OpenCppCoverage is an open source code coverage for C++.
// Copyright (C) 2017 OpenCppCoverage
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "stdafx.h"

#include "MonitoredLineRegister.hpp"

#include "ICoverageFilterManager.hpp"
#include "Address.hpp"
#include "BreakPoint.hpp"
#include "ExecutedAddressManager.hpp"
#include "CppCoverageException.hpp"
#include "FilterAssistant.hpp"

#include "FileFilter/ModuleInfo.hpp"
#include "FileFilter/FileInfo.hpp"
#include "FileFilter/LineInfo.hpp"

#include "Tools/PEFileHeader.hpp"
#include "Tools/ProcessMemory.hpp"
#include "Tools/Log.hpp"

namespace CppCoverage
{
	namespace
	{
		// The CLR header a managed or mixed-mode image points to from data
		// directory entry 14. Declared here because CorHdr.h is not part of the
		// Windows SDK. Only the first fields are needed.
		struct Cor20Header
		{
			DWORD cb;
			WORD MajorRuntimeVersion;
			WORD MinorRuntimeVersion;
			IMAGE_DATA_DIRECTORY MetaData;
			DWORD Flags;
		};

		const DWORD Cor20FlagsILOnly = 0x00000001;

		enum class ModuleType
		{
			// No CLR header at all.
			Native,
			// A CLR header without the IL-only flag: the image holds both
			// native machine code and managed code, as produced by /clr.
			MixedMode,
			// A CLR header with the IL-only flag: nothing to breakpoint.
			ManagedOnly
		};

		//---------------------------------------------------------------------
		std::wstring ToString(ModuleType moduleType)
		{
			switch (moduleType)
			{
				case ModuleType::Native: return L"native";
				case ModuleType::MixedMode: return L"mixed mode";
				case ModuleType::ManagedOnly: return L"managed";
			}
			return L"unknown";
		}

		struct ModuleKind : private Tools::IPEFileHeaderHandler
		{
			//----------------------------------------------------------------------------
			ModuleType GetModuleType(HANDLE hProcess, DWORD64 baseOfImage)
			{
				Tools::PEFileHeader fileHeader;

				fileHeader.Load(hProcess, baseOfImage, *this);
				return moduleType_;
			}

		  private:
			//-----------------------------------------------------------------
			template <typename T_IMAGE_NT_HEADERS>
			void OnNtHeader(HANDLE hProcess,
			                DWORD64 baseOfImage,
			                const T_IMAGE_NT_HEADERS& ntHeaders)
			{
				const auto& optionalHeader = ntHeaders.OptionalHeader;
				auto dataDirectory =
				    optionalHeader
				        .DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
				if (dataDirectory.VirtualAddress == 0 && dataDirectory.Size == 0)
				{
					moduleType_ = ModuleType::Native;
					return;
				}

				moduleType_ = IsILOnly(hProcess, baseOfImage, dataDirectory)
				                  ? ModuleType::ManagedOnly
				                  : ModuleType::MixedMode;
			}

			//-----------------------------------------------------------------
			static bool IsILOnly(HANDLE hProcess,
			                     DWORD64 baseOfImage,
			                     const IMAGE_DATA_DIRECTORY& dataDirectory)
			{
				if (dataDirectory.Size < sizeof(Cor20Header))
					return true;

				auto cor20Header = Tools::ReadStructInProcessMemory<Cor20Header>(
				    hProcess, baseOfImage + dataDirectory.VirtualAddress);

				return (cor20Header->Flags & Cor20FlagsILOnly) != 0;
			}

			//-----------------------------------------------------------------
			void OnNtHeader32(HANDLE hProcess,
			                  DWORD64 baseOfImage,
			                  const IMAGE_NT_HEADERS32& ntHeader) override
			{
				OnNtHeader(hProcess, baseOfImage, ntHeader);
			}

			//-----------------------------------------------------------------
			void OnNtHeader64(HANDLE hProcess,
			                  DWORD64 baseOfImage,
			                  const IMAGE_NT_HEADERS64& ntHeader) override
			{
				OnNtHeader(hProcess, baseOfImage, ntHeader);
			}

			ModuleType moduleType_ = ModuleType::Native;
		};
	}

	//----------------------------------------------------------------------------
	MonitoredLineRegister::MonitoredLineRegister(
	    std::shared_ptr<BreakPoint> breakPoint,
	    std::shared_ptr<ExecutedAddressManager> executedAddressManager,
	    std::shared_ptr<ICoverageFilterManager> coverageFilterManager,
	    std::unique_ptr<DebugInformationEnumerator> debugInformationEnumerator,
	    std::shared_ptr<FilterAssistant> filterAssistant,
	    bool allowMixedModeModules)
	    : breakPoint_{breakPoint},
	      executedAddressManager_{executedAddressManager},
	      coverageFilterManager_{coverageFilterManager},
	      debugInformationEnumerator_{std::move(debugInformationEnumerator)},
	      filterAssistant_{std::move(filterAssistant)},
	      allowMixedModeModules_{allowMixedModeModules}
	{
	}

	//----------------------------------------------------------------------------
	MonitoredLineRegister::~MonitoredLineRegister() = default;

	//----------------------------------------------------------------------------
	bool MonitoredLineRegister::RegisterLineToMonitor(
	    const std::filesystem::path& modulePath,
	    HANDLE hProcess,
	    void* baseOfImage)
	{
		auto moduleType = ModuleKind{}.GetModuleType(
		    hProcess, reinterpret_cast<DWORD64>(baseOfImage));

		if (moduleType == ModuleType::ManagedOnly ||
		    (moduleType == ModuleType::MixedMode && !allowMixedModeModules_))
		{
			LOG_INFO << modulePath.wstring() << " is skipped as it is a "
			         << ToString(moduleType) << " module.";
			if (moduleType == ModuleType::MixedMode)
			{
				LOG_INFO << L"Use --allow_mixed_mode_modules to cover the "
				            L"native code of this module.";
			}
			return false;
		}

		executedAddressManager_->AddModule(modulePath.wstring(), baseOfImage);

		moduleInfo_ = std::make_unique<FileFilter::ModuleInfo>(
		    hProcess, modulePath, baseOfImage);

		return debugInformationEnumerator_->Enumerate(modulePath, *this);
	}

	//--------------------------------------------------------------------------
	bool MonitoredLineRegister::IsSourceFileSelected(
	    const std::filesystem::path& path)
	{
		auto isSelected = coverageFilterManager_->IsSourceFileSelected(path.wstring());
		filterAssistant_->OnNewSourceFile(path, isSelected);
		return isSelected;
	}

	//--------------------------------------------------------------------------
	void
	MonitoredLineRegister::OnSourceFile(const std::filesystem::path& path,
	                                    const std::vector<Line>& lines)
	{
		std::vector<FileFilter::LineInfo> lineInfos;

		for (const auto& line : lines)
		{
			lineInfos.emplace_back(
			    line.lineNumber_, line.virtualAddress_, line.symbolIndex_);
		}

		FileFilter::FileInfo fileInfo{path, std::move(lineInfos)};
		const auto& moduleInfo = GetModuleInfo();

		std::vector<DWORD64> addresses;
		LineNumberByAddress lineNumberByAddress;

		for (const auto& lineInfo : fileInfo.lineInfoColllection_)
		{
			auto lineNumber = lineInfo.lineNumber_;
			if (coverageFilterManager_->IsLineSelected(
			        moduleInfo, fileInfo, lineInfo))
			{
				auto addressValue =
				    lineInfo.virtualAddress_ +
				    reinterpret_cast<DWORD64>(moduleInfo.baseOfImage_);

				lineNumberByAddress[addressValue].push_back(lineNumber);
				addresses.push_back(addressValue);
			}
		}
		SetBreakPoint(path,
		              moduleInfo.hProcess_,
		              std::move(addresses),
		              lineNumberByAddress);
	}

	//--------------------------------------------------------------------------
	void MonitoredLineRegister::SetBreakPoint(
	    const std::filesystem::path& path,
	    HANDLE hProcess,
	    std::vector<DWORD64>&& addressCollection,
	    const LineNumberByAddress& lineNumberByAddress)
	{
		auto oldInstructions =
		    breakPoint_->SetBreakPoints(hProcess, std::move(addressCollection));
		for (const auto& value : oldInstructions)
		{
			auto oldInstruction = value.first;
			const auto& addressValue = value.second;

			auto it = lineNumberByAddress.find(addressValue);
			if (it != lineNumberByAddress.end())
			{
				Address address{hProcess,
				                reinterpret_cast<void*>(addressValue)};
				const auto& lineNumbers = it->second;
				for (auto lineNumber : lineNumbers)
				{
					if (!executedAddressManager_->RegisterAddress(
					        address,
					        path.wstring(),
					        lineNumber,
					        oldInstruction))
					{
						breakPoint_->RemoveBreakPoint(address, oldInstruction);
					}
				}
			}
		}
	}

	//--------------------------------------------------------------------------
	const FileFilter::ModuleInfo& MonitoredLineRegister::GetModuleInfo() const
	{
		if (!moduleInfo_)
			THROW("moduleInfo_ is null.");
		return *moduleInfo_;
	}
}
