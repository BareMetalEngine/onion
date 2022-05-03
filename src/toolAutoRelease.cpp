#include "common.h"
#include "utils.h"
#include "toolAutoRelease.h"
#include "toolRelease.h"
#include "git.h"

//--

ToolAutoRelease::ToolAutoRelease()
{}

static bool AutoRelease_GetNextVersion(GitHubConfig& git, const Commandline& cmdline, std::string& outVersion)
{
	// get current release number
	uint32_t releaseNumber = 0;
	GitApi_GetHighestReleaseNumber(git, "v", 1, releaseNumber);
	std::cerr << "Current incremental release number is: " << releaseNumber << "\n";

	// assemble final version
	std::stringstream txt;
	txt << "v";
	txt << (releaseNumber+1);

	outVersion = txt.str();
	return true;
}

int ToolAutoRelease::run(const char* argv0, const Commandline& cmdline)
{
	const auto builderExecutablePath = fs::absolute(argv0);
	if (!fs::is_regular_file(builderExecutablePath))
	{
		std::cerr << KRED << "[BREAKING] Invalid local executable name: " << builderExecutablePath << "\n" << RST;
		return -1;
	}

	GitHubConfig git;
	if (!git.init(cmdline))
	{
		std::cerr << KRED << "[BREAKING] Failed to initialize GitHub helper\n" << RST;
		return -1;
	}

	//--

	const auto rootPath = builderExecutablePath.parent_path().parent_path().make_preferred();
	const auto rootGitPath = (rootPath / ".git").make_preferred();
	if (!fs::is_directory(rootGitPath))
	{
		std::cerr << KRED << "[BREAKING] Not running under git\n" << RST;
		return -1;
	}

	//--

#ifdef _WIN32
	{
		std::stringstream txt;
		txt << "git add -f bin/onion.exe";
		if (!RunWithArgsInDirectory(rootPath, txt.str()))
			return -1;
	}

	{
		std::stringstream txt;
		txt << "git commit -m \"[skip ci] Updated Onion Windows executables\"";
		if (!RunWithArgsInDirectory(rootPath, txt.str()))
			return -1;
	}
#else
	{
		std::stringstream txt;
		txt << "git add -f bin/onion";
		if (!RunWithArgsInDirectory(rootPath, txt.str()))
			return -1;
	}

	{
		std::stringstream txt;
		txt << "git commit -m \"[skip ci] Updated Onion Linux executables\"";
		if (!RunWithArgsInDirectory(rootPath, txt.str()))
			return -1;
	}
#endif

	{
		std::stringstream txt;
		txt << "git push";
		if (!RunWithArgsInDirectory(rootPath, txt.str()))
			return -1;
	}

	//--

	/*const auto rootPath = builderExecutablePath.parent_path().parent_path();
	const auto tempPublishPath = (rootPath / ".publish").make_preferred();
	const auto tempPublishPathInternal = (tempPublishPath / ".onion").make_preferred();
	if (!CreateDirectories(tempPublishPath))
	{
		std::cerr << KRED << "[BREAKING] Failed to create publish directory\n" << RST;
		return -1;
	}

	{
		bool valid = true;
		uint32_t totalCopied = 0;
		valid &= CopyNewerFilesRecursive(rootPath / "bin", tempPublishPathInternal / "bin", &totalCopied);
		valid &= CopyNewerFilesRecursive(rootPath / "cmake", tempPublishPathInternal / "cmake", &totalCopied);
		valid &= CopyNewerFilesRecursive(rootPath / "tools", tempPublishPathInternal / "tools", &totalCopied);
		valid &= CopyNewerFilesRecursive(rootPath / "uwp", tempPublishPathInternal / "uwp", &totalCopied);
		valid &= CopyNewerFilesRecursive(rootPath / "vs", tempPublishPathInternal / "vs", &totalCopied);
		if (!valid)
		{
			std::cerr << KRED << "[BREAKING] Not all files copied correctly\n" << RST;
			return -1;
		}

		std::cout << "Copied " << totalCopied << " files\n";
	}

	//--

	// archive path
	const auto archivePath = (tempPublishPath / "onion.zip").make_preferred();

	// package
	{
		std::stringstream command;
		command << "tar -acf ";
		command << EscapeArgument(archivePath.u8string());
		command << " .onion";

		if (!RunWithArgsInDirectory(tempPublishPath, command.str()))
		{
			std::cout << KRED << "Failed to package " << archivePath << "\n" << RST;
			return false;
		}
	}

	//--

	// determine next version info
	/*std::string releaseName;
	if (!Release_GetNextVersion(git, cmdline, releaseName))
	{
		std::cerr << KRED << "[BREAKING] Unable to determine next release tag\n" << RST;
		return false;
	}

	// create DRAFT release
	std::string releaseId;
	if (!GitApi_CreateRelease(git, releaseName, releaseName, "Automatic release", true, false, releaseId))
	{
		std::cerr << KRED << "[BREAKING] Failed to create GitHub release '" << releaseName << "'\n" << RST;
		return false;
	}*/

	return 0;
}

//--