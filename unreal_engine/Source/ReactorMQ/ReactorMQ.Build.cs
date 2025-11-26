using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class ReactorMQ : ModuleRules
{
	public ReactorMQ(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			[
				"Core",
				"CoreUObject"
			]
		);

		PrivateDependencyModuleNames.AddRange(
			[
				"Engine",
				"Json",
				"JsonUtilities",
				"Networking",
				"Projects",
				"SSL",
				"Sockets",
				"WebSockets"
			]
		);

		CppStandard = CppStandardVersion.Cpp20;

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Native", "include"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Native", "src"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Native", "fixtures"));

		var defs = new List<string>
		{
			"REACTORMQ_WITH_THREADS=1",
			"REACTORMQ_WITH_EXCEPTIONS=0",
			"REACTORMQ_WITH_RTTI=0",
			"REACTORMQ_WITH_CONSOLE_SINK=0",
			"REACTORMQ_WITH_FILE_SINK=0",
			"REACTORMQ_WITH_UE_LOG_SINK=1",
			"REACTORMQ_THREAD=0",

			// UE5 context: OpenSSL handled by UBT's SSL module
			"REACTORMQ_WITH_TLS=1",

			"REACTORMQ_WITH_O3DE=0",
			"REACTORMQ_WITH_UE5=1",

			// Secure socket selection
			"REACTORMQ_SECURE_SOCKET_WITH_TLS=0",
			"REACTORMQ_SECURE_SOCKET_WITH_BIO_TLS=1",
			"REACTORMQ_SOCKET_WITH_POSIX_SOCKET=0",
			"REACTORMQ_SOCKET_WITH_WINSOCKET=0",
			"REACTORMQ_SOCKET_WITH_SONY_SOCKET=0",
			"REACTORMQ_SOCKET_WITH_IOCTL=0",
			"REACTORMQ_SOCKET_WITH_POLL=0",
			"REACTORMQ_SOCKET_WITH_SELECT=0",
			"REACTORMQ_SOCKET_WITH_GETHOSTNAME=0",
			"REACTORMQ_SOCKET_WITH_GETADDRINFO=0",
			"REACTORMQ_SOCKET_WITH_GETNAMEINFO=0",
			"REACTORMQ_SOCKET_WITH_CLOSE_ON_EXEC=0",
			"REACTORMQ_SOCKET_WITH_MSG_DONTWAIT=0",
			"REACTORMQ_SOCKET_WITH_RECVMMSG=0",
			"REACTORMQ_SOCKET_WITH_TIMESTAMP=0",
			"REACTORMQ_SOCKET_WITH_NODELAY=0",
			"REACTORMQ_PLATFORM_HAS_BSD_TIME=0",
			"REACTORMQ_PLATFORM_HAS_BSD_IPV6_SOCKETS=0"
		};

		// Platform mapping
		var platformName = Target.Platform.ToString();
		var isWindows = Target.Platform == UnrealTargetPlatform.Win64 || platformName == "WinArm64";
		var isLinux = Target.Platform == UnrealTargetPlatform.Linux;
		var isMac = Target.Platform == UnrealTargetPlatform.Mac;
		var isIos = Target.Platform == UnrealTargetPlatform.IOS;
		var isAndroid = Target.Platform == UnrealTargetPlatform.Android;
		var isPs5 = platformName == "PS5"; // Prospero
		var isSwitch = platformName == "Switch" || platformName == "NX";
		var isXsx = platformName == "XSX";

		var isDarwinFamily = isMac || isIos;
		var isWindowsFamily = isWindows || isXsx;
		var isConsoleFamily = isPs5 || isXsx || isSwitch;
		var isPosixFamily = isLinux || isAndroid || isDarwinFamily || isPs5 || isSwitch;

		// Platform defines
		defs.Add($"REACTORMQ_PLATFORM_WINDOWS={(isWindows ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_LINUX={(isLinux ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_ANDROID={(isAndroid ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_MACOS={(isMac ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_IOS={(isIos ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_PROSPERO={(isPs5 ? 1 : 0)}");

		defs.Add($"REACTORMQ_PLATFORM_WINDOWS_FAMILY={(isWindowsFamily ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_DARWIN_FAMILY={(isDarwinFamily ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_POSIX_FAMILY={(isPosixFamily ? 1 : 0)}");
		defs.Add($"REACTORMQ_PLATFORM_CONSOLE_FAMILY={(isConsoleFamily ? 1 : 0)}");

		// Feature-probe style flags (approximation of CMake CheckSymbolExists)
		// Windows family lacks unistd/utsname, POSIX family typically has them.
		defs.Add($"REACTORMQ_WITH_GETHOSTNAME={(isPosixFamily ? 1 : 0)}");
		defs.Add($"REACTORMQ_WITH_UNAME={(isDarwinFamily || isLinux ? 1 : 0)}");

		// Windows-family overrides
		if (isWindowsFamily)
		{
			PrivatePCHHeaderFile = "ReactorMQPCH.h";
		}

		// Darwin-family overrides
		if (isDarwinFamily)
		{
			// 
		}

		// Linux overrides
		if (isLinux)
		{
			// 
		}

		// Android overrides
		if (isAndroid)
		{
			//
		}

		// PS5 (Prospero) overrides
		if (isPs5)
		{
			// 
		}

		PrivateDefinitions.AddRange(defs);
	}
}