#include <cstdlib>
#include <string>

int main() {
    // Replace "a.1" with the actual name of your compiled man page
    const char* manPageName = "../man1/template.1";

    // Construct the command to call 'man' on the compiled man page
    std::string command = "man " + std::string(manPageName);

    // Use the system function to execute the 'man' command
    int result = std::system(command.c_str());

    // Check the result of the 'man' command
    if (result == 0) {
        // Success
        return 0;
    } else {
        // Error
        return 1;
    }
}
