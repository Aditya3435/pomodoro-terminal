#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Global Constants
const int SHORT_WORK = 30;
const int SHORT_BREAK = 5;
const int MID_WORK = 90;
const int MID_BREAK = 15;
const int LONG_WORK = 180;
const int LONG_BREAK = 30;

const string NOTIFY_SEND_CMD = "notify-send \"Pomodoro Timer\" \"";
const string SOUND_LOGIN = "paplay /usr/share/sounds/freedesktop/stereo/service-login.oga";
const string SOUND_LOGOUT = "paplay /usr/share/sounds/freedesktop/stereo/service-logout.oga";
const string SOUND_COMPLETE = "paplay /usr/share/sounds/freedesktop/stereo/complete.oga";

// Macros for Colors
#define RESET   "\033[0m"
#define BLUE    "\033[34m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define RED     "\033[31m"
#define MAGENTA "\033[35m"
#define WHITE   "\033[37m"

#define PRINT_COLORED(text, color) cout << "\033[48;5;0m\033[" << color  << text << "\033[0m" << endl
#define PRINT_SEPARATOR(color) cout << "\033[48;5;0m\033[" << color << "———————————————————————————————————————————————\033[0m" << endl

// Global Variables
int totalFocusTime = 0;
string suggestions[] = {
        "Stand up and stretch your body.",
        "Take a short walk to get your blood flowing.",
        "Practice deep breathing exercises for a minute.",
        "Rest your eyes by looking away from your screen.",
        "Drink a glass of water to stay hydrated.",
        "Do some gentle neck and shoulder stretches.",
        "Take a moment to practice mindfulness.",
        "Step outside for a few minutes of fresh air.",
        "Re-adjust your posture and sit comfortably.",
        "Organize your desk for a cleaner workspace."};


// Utility Functions
vector<string> loadQuotesFromFile(const string& fileName) {
    vector<string> quotes;
    if (fs::exists(fileName)) {
        ifstream file(fileName);
        string line;
        while (getline(file, line)) {
            quotes.push_back(line);
        }
        file.close();
    } else {
        cout << "Error: Could not find the quotes file!" << endl;
    }
    return quotes;
}
string getRandomQuote(const vector<string>& quotes) {
    if (!quotes.empty()) {
        srand(time(0));  // Seed the random number generator
        int randomIndex = rand() % quotes.size();  // Random index for a quote
        return quotes[randomIndex];
    }
    return "No quotes available.";
}

// to get a single character input without pressing Enter
char getch() {
    char buf = 0;
    struct termios old = {};
    if (tcgetattr(STDIN_FILENO, &old) < 0)
        perror("tcgetattr");
    struct termios newt = old;
    newt.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0)
        perror("tcsetattr");
    if (read(STDIN_FILENO, &buf, 1) < 0)
        perror("read");
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0)
        perror("tcsetattr");
    return buf;
}

// clickable hyperlink 
string clickableLink(const string &text, const string &url) {
    return "\033]8;;" + url + "\033\\" + text + "\033]8;;\033\\";
}

// to print intro screen with colorful ASCII art
void printIntro() {
    system("clear"); 

    string asciiArt[] = {
        " _____                              _                      _    _                         ",
        "|  __ \\                            | |                    | |  (_)                       ",
        "| |__) |___   _ __ ___    ___    __| |  ___   _ __  ___   | |_  _  _ __ ___    ___  _ __ ",
        "|  ___// _ \\ | '_ ` _ \\  / _ \\  / _` | / _ \\ | '__|/ _ \\  | __|| || '_ ` _ \\  / _ \\| '__|",
        "| |   | (_) || | | | | || (_) || (_| || (_) || |  | (_) | | |_ | || | | | | ||  __/| |   ",
        "|_|    \\___/ |_| |_| |_| \\___/  \\__,_| \\___/ |_|   \\___/   \\__||_||_| |_| |_| \\___||_|   \n"};

    int asciiArtLines = sizeof(asciiArt) / sizeof(asciiArt[0]);

    for (int i = 0; i < asciiArtLines; i++) {
        PRINT_COLORED(asciiArt[i], MAGENTA);
    }

    string devVisibleText = "~Developed by Aditya Maurya";
    string devText = clickableLink(devVisibleText, "https://github.com/Aditya3435/");
    PRINT_COLORED(devText, GREEN);
    cout << endl;

    PRINT_COLORED("Press any key to continue...", CYAN);
    getch(); // Wait for any key press
}

// to send a desktop notification and play a sound
void notify(const string &message, const string &sound) {
    string command = NOTIFY_SEND_CMD + message + "\"";
    system(command.c_str());
    system(sound.c_str());
}

// to update the UI with progress and time left
void updateUI(int totalSeconds, int elapsedSeconds) {
    int progressBars = (elapsedSeconds * 40) / totalSeconds;
    int percentage = (elapsedSeconds * 100) / totalSeconds;
    string progressLine = "[";
    for (int j = 0; j < 40; ++j) {
        progressLine += (j < progressBars ? "#" : "-");
    }
    progressLine += "] " + to_string(percentage) + "%";

    int remaining = totalSeconds - elapsedSeconds;
    int hours = remaining / 3600;
    int minutes = (remaining % 3600) / 60;
    int seconds = remaining % 60;
    stringstream timeStream;
    timeStream << "Time Left: " << setw(2) << setfill('0') << hours << ":"
               << setw(2) << setfill('0') << minutes << ":"
               << setw(2) << setfill('0') << seconds;
    string timeLine = timeStream.str();

    // Move the cursor up two lines and clear them
    cout << "\033[2A"; // Move cursor up 2 lines
    PRINT_COLORED(progressLine, WHITE);
    PRINT_COLORED(timeLine, WHITE);

    cout.flush();
}

void countdown(int minutes) {
    int totalSeconds = minutes * 60;
    cout << "\033[?25l"; // Hide the cursor
    cout << endl;
    PRINT_COLORED("Press Ctrl + C to exit the timer...", RED);

    // Print two blank lines initially to reserve space for the progress UI
    cout << "\n\n";
    for (int elapsed = 0; elapsed <= totalSeconds; ++elapsed) {
        updateUI(totalSeconds, elapsed);
        this_thread::sleep_for(chrono::seconds(1));
    }

    cout << "\033[?25h" << endl; // Show the cursor again
}

// to save the session to a file
void saveSession(int workMinutes) {
    totalFocusTime += workMinutes;
    ofstream file("pomodoro_history.txt", ios::app);
    if (file.is_open()) {
        file << "Completed a " << workMinutes << "-minute session on "
             << __DATE__ << " at " << __TIME__ << "\n";
        file.close();
    }
}

// to show random break suggestions
void showBreakSuggestions() {
    int suggestionCount = sizeof(suggestions) / sizeof(suggestions[0]);
    int randomIndex = rand() % suggestionCount;
    PRINT_COLORED("Break Suggestion: " + suggestions[randomIndex], MAGENTA);
}

// to start a Pomodoro session
void startPomodoro(int workMinutes, int breakMinutes, string& randomQuote) {

    PRINT_SEPARATOR(CYAN);
    PRINT_COLORED("Work for " + to_string(workMinutes) + " minutes!", CYAN);
    PRINT_COLORED(randomQuote, MAGENTA);
    notify("Time to focus! Work session started.", SOUND_LOGIN);
    countdown(workMinutes);
    saveSession(workMinutes);


    PRINT_SEPARATOR(CYAN);
    PRINT_COLORED("Break for " + to_string(breakMinutes) + " minutes!", CYAN);
    notify("Break time! Relax.", SOUND_COMPLETE);

    showBreakSuggestions();
    countdown(breakMinutes);
}

// to display the daily focus time report
void displayDailyReport() {
    PRINT_SEPARATOR(MAGENTA);
    PRINT_COLORED("Today's Total Focus Time: " + to_string(totalFocusTime) + " minutes", MAGENTA);
    PRINT_SEPARATOR(MAGENTA);
}


int main() {
    cout << "\033[48;5;0m"; // Set background color to black
    cout << "\033[2J";      // Clear the screen
    printIntro();

    int choice, workMinutes, breakMinutes;

    PRINT_SEPARATOR(CYAN);
    PRINT_COLORED("Choose a Pomodoro session:", CYAN);
    PRINT_COLORED("1. Short-term (30 min work / 5 min break)", YELLOW);
    PRINT_COLORED("2. Mid-term (90 min work / 15 min break)", YELLOW);
    PRINT_COLORED("3. Long-term (180 min work / 30 min break)", YELLOW);
    PRINT_COLORED("4. Custom Timer", YELLOW);

    PRINT_SEPARATOR(CYAN);
    cout << "\033[48;5;0m\033[" << CYAN << "Enter your choice: " << "\033[0m";
    cin >> choice;


    string fileName = "quotes.txt";
    vector<string> quotes = loadQuotesFromFile(fileName);
    string randomQuote = getRandomQuote(quotes);



    switch (choice) {
        case 1:
            workMinutes = SHORT_WORK;
            breakMinutes = SHORT_BREAK;
            break;
        case 2:
            workMinutes = MID_WORK;
            breakMinutes = MID_BREAK;
            break;
        case 3:
            workMinutes = LONG_WORK;
            breakMinutes = LONG_BREAK;
            break;
        case 4:
            cout << "\033[48;5;0m\033[" << CYAN << "Enter work duration (in minutes): " << "\033[0m";
            cin >> workMinutes;
            cout << "\033[48;5;0m\033[" << CYAN << "Enter break duration (in minutes): " << "\033[0m";
            cin >> breakMinutes;
            break;
        default:
            PRINT_COLORED("Invalid choice! Exiting...", RED);
            return 1;
    }

    startPomodoro(workMinutes, breakMinutes, randomQuote);
    displayDailyReport();
    notify("Start the timer again!", SOUND_LOGOUT);

    return 0;
}
