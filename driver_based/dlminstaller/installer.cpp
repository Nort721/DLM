#include "instutil.h"

int main(int argc, char** argv)
{
	if (argc > 1 && strcmp(argv[1], "uninstall") == 0) {
		std::cout << "Uninstalling DriverLoadMonitoring . . .\n";

		if (!UninstallDLM()) {
			std::cerr << "The attempt to uninstall DLM has failed.\n";
			return 1;
		}

		std::cout << "DLM has been uninstalled successfully.\n";
	}
	else {
		std::cout << "Installing DriverLoadMonitoring . . .\n";

		if (!InstallDLM()) {
			std::cerr << "The attempt to install DLM has failed.\n";
			return 1;
		}

		std::cout << "DLM has been installed successfully.\n";
	}

	return 0;
}