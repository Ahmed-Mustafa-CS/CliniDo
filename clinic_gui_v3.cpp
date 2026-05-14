/*
=============================================================
   CliniDo - Online Clinic Appointment System
   GUI built with SFML 3 — Inspired by Clinido.com style
   v3: Welcome Screen + Patient Choice + Book Now in View Doctors + Rate Doctor + Years of Experience
=============================================================

HOW TO COMPILE:
  C:\msys64\ucrt64\bin\g++ clinic_gui_v3.cpp -o clinic.exe -I"C:\msys64\ucrt64\include" -L"C:\msys64\ucrt64\lib" -lsfml-graphics -lsfml-window -lsfml-system

REQUIRED FILES in same folder:
  - arial.ttf
  - logo.png

=============================================================
*/

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <optional>
using namespace std;

// ============================================================
// ======================== COLORS ============================
// ============================================================
sf::Color CLINI_BLUE(30, 136, 229);
sf::Color CLINI_DARK(13, 71, 161);
sf::Color CLINI_LIGHT_BG(240, 248, 255);
sf::Color CLINI_WHITE(255, 255, 255);
sf::Color CLINI_GRAY(189, 189, 189);
sf::Color CLINI_DARK_TEXT(33, 33, 33);
sf::Color CLINI_GREEN(76, 175, 80);
sf::Color CLINI_RED(229, 57, 53);
sf::Color CLINI_SHADOW(220, 230, 241);
sf::Color CLINI_CYAN(41, 182, 246);

// ============================================================
// ======================== STRUCTS ===========================
// ============================================================
struct Phone { string number; };

struct Patient {
    int id = 0;
    string name, password, disease;
    int age = 0;
    Phone phones[3];
    int phoneCount = 0;
};

// shift: 0 = Morning (06:00-12:00), 1 = Afternoon (12:00-18:00), 2 = Evening (18:00-24:00)
struct Doctor {
    int id = 0;
    string name, specialization;
    int slots = 0, availableSlots = 0;
    float consultationFee = 0;
    int shift = 0;         // 0=Morning, 1=Afternoon, 2=Evening
    int checkInHour = -1;  // hour the doctor checked in (24h), -1 = not checked in
    bool checkedOut = false;
    int yearsOfExperience = 0;
    float rating = 0.0f;       // average rating out of 5
    int ratingCount = 0;       // number of ratings submitted
    int totalBookings = 0;     // total bookings ever made with this doctor
};

struct Appointment {
    int appointmentID = 0, patientID = 0, doctorID = 0;
    string date, time, status;
    float totalCost = 0;
};

struct Admin {
    int adminID = 0;
    string name, password;
};

// ============================================================
// ======================== ARRAYS ============================
// ============================================================
#define MAX_PATIENTS 50
#define MAX_DOCTORS 20
#define MAX_APPOINTMENTS 100
#define MAX_ADMINS 2

Patient patients[MAX_PATIENTS];
Doctor doctors[MAX_DOCTORS];
Appointment appointments[MAX_APPOINTMENTS];
Admin admins[MAX_ADMINS];

int patientCount = 0, doctorCount = 0, appointmentCount = 0, adminCount = 0;
int loggedInPatientID = -1;

// ============================================================
// ===================== SCREENS ==============================
// ============================================================
enum Screen {
    SCREEN_MAIN,
    SCREEN_PATIENT_CHOICE,      // NEW: patient sees Log In / Sign Up choice
    SCREEN_PATIENT_LOGIN,
    SCREEN_PATIENT_SIGNUP,
    SCREEN_PATIENT_MENU,
    SCREEN_VIEW_DOCTORS,
    SCREEN_SELECT_DOCTOR,       // NEW: patient picks a doctor
    SCREEN_BOOK_SLOT,           // NEW: patient sees visual time grid
    SCREEN_CANCEL_APPOINTMENT,
    SCREEN_VIEW_HISTORY,
    SCREEN_RATE_DOCTOR,         // NEW: patient rates a doctor
    SCREEN_FIND_APPOINTMENT,
    SCREEN_ADMIN_LOGIN,
    SCREEN_ADMIN_MENU,
    SCREEN_ADMIN_ADD_DOCTOR,
    SCREEN_ADMIN_EDIT_SLOTS,
    SCREEN_ADMIN_REMOVE_DOCTOR,
    SCREEN_ADMIN_VIEW_APPOINTMENTS,
    SCREEN_ADMIN_REPORTS,
    SCREEN_ADMIN_DOCTOR_CHECKOUT  // NEW: admin checks out a doctor
};

Screen currentScreen = SCREEN_MAIN;
string statusMessage = "";
bool statusIsError = false;

// ============================================================
// =================== SHIFT HELPERS ==========================
// ============================================================
// Returns the start hour (24h) of a shift
int shiftStartHour(int shift) {
    if (shift == 0) return 6;
    if (shift == 1) return 12;
    return 18;
}

// Returns the end hour (exclusive) of a shift
int shiftEndHour(int shift) {
    if (shift == 0) return 12;
    if (shift == 1) return 18;
    return 24;
}

// Returns display label for a shift
string shiftLabel(int shift) {
    if (shift == 0) return "Morning  (06:00 - 12:00)";
    if (shift == 1) return "Afternoon (12:00 - 18:00)";
    return "Evening  (18:00 - 24:00)";
}

// Returns short shift name
string shiftName(int shift) {
    if (shift == 0) return "Morning";
    if (shift == 1) return "Afternoon";
    return "Evening";
}

// Returns the time string for slot index (0-5) in a shift
// Each shift has 6 x 1-hour slots
string slotTimeStr(int shift, int slotIdx) {
    int h = shiftStartHour(shift) + slotIdx;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:00", h);
    return string(buf);
}

// Check if a slot is booked for a given doctor + date + slotIdx
bool isSlotBooked(int doctorID, const string& date, int shift, int slotIdx) {
    string t = slotTimeStr(shift, slotIdx);
    for (int i = 0; i < appointmentCount; i++) {
        if (appointments[i].doctorID == doctorID &&
            appointments[i].date == date &&
            appointments[i].time == t &&
            appointments[i].status == "Active")
            return true;
    }
    return false;
}

// ============================================================
// =================== DATE VALIDATION ========================
// ============================================================
// Minimum allowed date: 10/5/2026
// Parses "YYYY-MM-DD" format
bool isDateValid(const string& dateStr) {
    // Expected format: YYYY-MM-DD
    if (dateStr.size() != 10) return false;
    if (dateStr[4] != '-' || dateStr[7] != '-') return false;
    int year  = atoi(dateStr.substr(0, 4).c_str());
    int month = atoi(dateStr.substr(5, 2).c_str());
    int day   = atoi(dateStr.substr(8, 2).c_str());
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31) return false;

    // Compare against 2026-05-10
    if (year > 2026) return true;
    if (year < 2026) return false;
    if (month > 5)   return true;
    if (month < 5)   return false;
    return day >= 10;
}

// ============================================================
// =================== DATA FUNCTIONS =========================
// ============================================================
void loadInitialData() {
    patients[0] = {1, "Ahmed Ali",    "200720", "Heart",   30, {{"01075649205"}}, 1};
    patients[1] = {2, "Ali Mohamed",  "248065", "Stomach", 25, {{"01271340239"}}, 1};
    patientCount = 2;

    doctors[0] = {1, "Dr. Mohammed Adham", "Heart",   6, 6, 500, 0, -1, false, 10, 4.5f, 12, 24};
    doctors[1] = {2, "Dr. Adam Mohammed",  "Stomach", 6, 6, 200, 1, -1, false, 7,  4.2f, 8,  16};
    doctors[2] = {3, "Dr. Ibrahim Yaser",  "Heart",   6, 6, 500, 2, -1, false, 15, 4.8f, 20, 40};
    doctorCount = 3;

    admins[0] = {1, "Admin", "1234"};
    adminCount = 1;
}

void saveDataToFile() {
    ofstream pf("patients.txt");
    pf << patientCount << "\n";
    for (int i = 0; i < patientCount; i++) {
        pf << patients[i].id << "\n" << patients[i].name << "\n"
           << patients[i].password << "\n" << patients[i].disease << "\n"
           << patients[i].age << "\n" << patients[i].phoneCount << "\n";
        for (int j = 0; j < patients[i].phoneCount; j++)
            pf << patients[i].phones[j].number << "\n";
    }
    pf.close();

    ofstream df("doctors.txt");
    df << doctorCount << "\n";
    for (int i = 0; i < doctorCount; i++)
        df << doctors[i].id << "\n" << doctors[i].name << "\n"
           << doctors[i].specialization << "\n" << doctors[i].slots << "\n"
           << doctors[i].availableSlots << "\n" << doctors[i].consultationFee << "\n"
           << doctors[i].shift << "\n" << doctors[i].checkInHour << "\n"
           << doctors[i].checkedOut << "\n"
           << doctors[i].yearsOfExperience << "\n"
           << doctors[i].rating << "\n"
           << doctors[i].ratingCount << "\n"
           << doctors[i].totalBookings << "\n";
    df.close();

    ofstream af("appointments.txt");
    af << appointmentCount << "\n";
    for (int i = 0; i < appointmentCount; i++)
        af << appointments[i].appointmentID << "\n" << appointments[i].patientID << "\n"
           << appointments[i].doctorID << "\n" << appointments[i].date << "\n"
           << appointments[i].time << "\n" << appointments[i].status << "\n"
           << appointments[i].totalCost << "\n";
    af.close();
}

void loadDataFromFile() {
    ifstream pf("patients.txt");
    if (!pf.is_open()) { loadInitialData(); return; }
    pf >> patientCount; pf.ignore();
    for (int i = 0; i < patientCount; i++) {
        pf >> patients[i].id; pf.ignore();
        getline(pf, patients[i].name);
        getline(pf, patients[i].password);
        getline(pf, patients[i].disease);
        pf >> patients[i].age >> patients[i].phoneCount; pf.ignore();
        for (int j = 0; j < patients[i].phoneCount; j++)
            getline(pf, patients[i].phones[j].number);
    }
    pf.close();

    ifstream df("doctors.txt");
    if (df.is_open()) {
        df >> doctorCount; df.ignore();
        for (int i = 0; i < doctorCount; i++) {
            df >> doctors[i].id; df.ignore();
            getline(df, doctors[i].name);
            getline(df, doctors[i].specialization);
            df >> doctors[i].slots >> doctors[i].availableSlots
               >> doctors[i].consultationFee >> doctors[i].shift
               >> doctors[i].checkInHour >> doctors[i].checkedOut;
            df.ignore();
            // new fields (optional – default to 0 if old file)
            if (df >> doctors[i].yearsOfExperience) {
                df >> doctors[i].rating >> doctors[i].ratingCount >> doctors[i].totalBookings;
                df.ignore();
            } else { df.clear(); df.ignore(); }
        }
        df.close();
    }

    ifstream af("appointments.txt");
    if (af.is_open()) {
        af >> appointmentCount; af.ignore();
        for (int i = 0; i < appointmentCount; i++) {
            af >> appointments[i].appointmentID >> appointments[i].patientID >> appointments[i].doctorID;
            af.ignore();
            getline(af, appointments[i].date);
            getline(af, appointments[i].time);
            getline(af, appointments[i].status);
            af >> appointments[i].totalCost; af.ignore();
        }
        af.close();
    }

    admins[0] = {1, "Admin", "1234"};
    adminCount = 1;
}

int findAppointmentIndex(int id) {
    for (int i = 0; i < appointmentCount; i++)
        if (appointments[i].appointmentID == id) return i;
    return -1;
}

// ============================================================
// =================== UI HELPERS =============================
// ============================================================

void drawRect(sf::RenderWindow& win, float x, float y, float w, float h,
              sf::Color fill, sf::Color outline = sf::Color::Transparent, float thick = 0) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(fill);
    rect.setOutlineColor(outline);
    rect.setOutlineThickness(thick);
    win.draw(rect);
}

void drawText(sf::RenderWindow& win, sf::Font& font, const string& str,
              float x, float y, unsigned int size, sf::Color color,
              bool bold = false, bool center = false) {
    sf::Text text(font, str, size);
    text.setFillColor(color);
    if (bold) text.setStyle(sf::Text::Bold);
    if (center) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setPosition({x - bounds.size.x / 2.f, y});
    } else {
        text.setPosition({x, y});
    }
    win.draw(text);
}

struct Button {
    float x, y, w, h;
    string label;
    sf::Color bgColor;
    sf::Color textColor;
    bool isHovered = false;

    bool contains(float mx, float my) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
};

void drawButton(sf::RenderWindow& win, sf::Font& font, Button& btn) {
    sf::Color bg = btn.isHovered ?
        sf::Color(max(0, (int)btn.bgColor.r - 20),
                  max(0, (int)btn.bgColor.g - 20),
                  max(0, (int)btn.bgColor.b - 20)) :
        btn.bgColor;
    drawRect(win, btn.x, btn.y, btn.w, btn.h, bg);
    drawText(win, font, btn.label, btn.x + btn.w / 2.f, btn.y + btn.h / 2.f - 12, 17, btn.textColor, true, true);
}

struct InputField {
    float x, y, w, h;
    string label;
    string value;
    bool focused = false;
    bool isPassword = false;

    bool contains(float mx, float my) {
        return mx >= x && mx <= x + w && my >= y && my <= y + h;
    }
};

void drawInputField(sf::RenderWindow& win, sf::Font& font, InputField& field) {
    drawText(win, font, field.label, field.x, field.y - 22, 14, sf::Color(100, 100, 100));
    sf::Color border = field.focused ? CLINI_BLUE : CLINI_GRAY;
    drawRect(win, field.x, field.y, field.w, field.h, CLINI_WHITE, border, 2);
    string display = field.isPassword ? string(field.value.size(), '*') : field.value;
    if (field.focused) display += "|";
    drawText(win, font, display, field.x + 10, field.y + 12, 15, CLINI_DARK_TEXT);
}

void drawNavbar(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex) {
    drawRect(win, 0, 0, 1100, 65, CLINI_WHITE);
    drawRect(win, 0, 65, 1100, 2, CLINI_SHADOW);

    if (logoTex.getSize().x > 0) {
        sf::Sprite logo(logoTex);
        float scale = 45.f / logoTex.getSize().y;
        logo.setScale({scale, scale});
        logo.setPosition({20.f, 10.f});
        win.draw(logo);
    } else {
        drawText(win, font, "CliniDo", 20, 15, 26, CLINI_BLUE, true);
    }
}

void drawHero(sf::RenderWindow& win, sf::Font& font) {
    drawRect(win, 0, 67, 1100, 180, CLINI_BLUE);
    drawText(win, font, "Book Your Appointment Online", 550, 100, 28, CLINI_WHITE, true, true);
    drawText(win, font, "Find the best doctors and book in minutes", 550, 145, 17, sf::Color(200, 235, 255), false, true);
}

void drawCard(sf::RenderWindow& win, sf::Font& font, float x, float y, float w, float h,
              const string& title, const string& sub1, const string& sub2, const string& badge) {
    drawRect(win, x + 3, y + 3, w, h, CLINI_SHADOW);
    drawRect(win, x, y, w, h, CLINI_WHITE);
    drawRect(win, x, y, 5, h, CLINI_BLUE);

    sf::CircleShape circle(26);
    circle.setFillColor(CLINI_LIGHT_BG);
    circle.setOutlineColor(CLINI_CYAN);
    circle.setOutlineThickness(2);
    circle.setPosition({x + 14.f, y + h / 2.f - 26.f});
    win.draw(circle);

    drawText(win, font, title, x + 68, y + 12, 16, CLINI_DARK_TEXT, true);
    drawText(win, font, sub1,  x + 68, y + 36, 13, sf::Color(80, 80, 80));
    drawText(win, font, sub2,  x + 68, y + 56, 13, CLINI_BLUE);
    drawRect(win, x + w - 115, y + 12, 105, 26, CLINI_LIGHT_BG);
    drawText(win, font, badge, x + w - 62, y + 16, 12, CLINI_DARK, false, true);
}

void drawStatus(sf::RenderWindow& win, sf::Font& font) {
    if (statusMessage.empty()) return;
    sf::Color bg = statusIsError ? sf::Color(255, 235, 235) : sf::Color(232, 245, 233);
    sf::Color tc = statusIsError ? CLINI_RED : CLINI_GREEN;
    drawRect(win, 50, 695, 1000, 38, bg, statusIsError ? CLINI_RED : CLINI_GREEN, 1);
    drawText(win, font, statusMessage, 550, 703, 14, tc, false, true);
}

void setStatus(const string& msg, bool isError = false) {
    statusMessage = msg;
    statusIsError = isError;
}

// ============================================================
// ======================= SCREENS ============================
// ============================================================

void drawMainScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);

    // ---- Full-width hero banner ----
    drawRect(win, 0, 0, 1100, 280, CLINI_BLUE);
    // subtle diagonal accent
    sf::ConvexShape accent;
    accent.setPointCount(4);
    accent.setPoint(0, {700.f, 0.f});
    accent.setPoint(1, {1100.f, 0.f});
    accent.setPoint(2, {1100.f, 280.f});
    accent.setPoint(3, {900.f, 280.f});
    accent.setFillColor(CLINI_DARK);
    win.draw(accent);

    // Logo / brand name top-left
    if (logoTex.getSize().x > 0) {
        sf::Sprite logo(logoTex);
        float scale = 50.f / logoTex.getSize().y;
        logo.setScale({scale, scale});
        logo.setPosition({36.f, 24.f});
        win.draw(logo);
    } else {
        drawText(win, font, "CliniDo", 36, 24, 28, CLINI_WHITE, true);
    }

    // Welcome text
    drawText(win, font, "Welcome to CliniDo", 550, 80, 36, CLINI_WHITE, true, true);
    drawText(win, font, "Your trusted online clinic appointment system", 550, 132, 18, sf::Color(200, 235, 255), false, true);
    drawText(win, font, "Fast  |  Reliable  |  Professional", 550, 165, 14, sf::Color(170, 210, 255), false, true);

    // ---- Stats row (right side of hero) ----
    // Count nice numbers
    auto niceNum = [](int n) -> string {
        if (n >= 1000) return to_string(n/1000) + "K+";
        if (n >= 100)  return to_string((n/10)*10) + "+";
        if (n >= 10)   return to_string((n/5)*5) + "+";
        return to_string(n) + "+";
    };

    // Count active appointments
    int activeAppts = 0;
    for (int i = 0; i < appointmentCount; i++)
        if (appointments[i].status == "Active") activeAppts++;

    struct Stat { string val; string label; };
    Stat stats[3] = {
        { niceNum(patientCount), "Registered\nPatients" },
        { niceNum(doctorCount),  "Expert\nDoctors"      },
        { niceNum(activeAppts),  "Appointments\nBooked"  }
    };

    float sx = 640, sy = 72, sw = 140, sh = 110, sgap = 10;
    for (int i = 0; i < 3; i++) {
        float cx = sx + i * (sw + sgap);
        drawRect(win, cx, sy, sw, sh, sf::Color(255,255,255,30));
        drawText(win, font, stats[i].val, cx + sw/2.f, sy + 18, 30, CLINI_WHITE, true, true);
        // split label on \n
        size_t nl = stats[i].label.find('\n');
        string l1 = stats[i].label.substr(0, nl);
        string l2 = stats[i].label.substr(nl+1);
        drawText(win, font, l1, cx + sw/2.f, sy + 64, 12, sf::Color(200,235,255), false, true);
        drawText(win, font, l2, cx + sw/2.f, sy + 82, 12, sf::Color(200,235,255), false, true);
    }

    // ---- Portal selection cards ----
    drawText(win, font, "Choose how you'd like to continue:", 550, 300, 16, sf::Color(80,80,80), false, true);

    // Admin card
    drawRect(win, 175, 330, 330, 170, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 175, 330, 330, 5, CLINI_DARK);
    drawText(win, font, "Admin", 340, 350, 20, CLINI_DARK, true, true);
    drawText(win, font, "Manage doctors, appointments", 340, 384, 13, sf::Color(100,100,100), false, true);
    drawText(win, font, "and view system reports.", 340, 402, 13, sf::Color(100,100,100), false, true);

    // Patient card
    drawRect(win, 595, 330, 330, 170, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 595, 330, 330, 5, CLINI_CYAN);
    drawText(win, font, "Patient", 760, 350, 20, CLINI_DARK, true, true);
    drawText(win, font, "Book appointments and manage", 760, 384, 13, sf::Color(100,100,100), false, true);
    drawText(win, font, "your health visits.", 760, 402, 13, sf::Color(100,100,100), false, true);

    buttons.clear();
    // Admin button (inside admin card)
    buttons.push_back({215, 450, 250, 42, "Admin Login", CLINI_DARK, CLINI_WHITE});
    // Patient button (inside patient card)
    buttons.push_back({635, 450, 250, 42, "Get Started", CLINI_CYAN, CLINI_WHITE});
    // Exit
    buttons.push_back({440, 680, 220, 40, "Exit", CLINI_RED, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

vector<InputField> loginFields;
void initLoginFields() {
    loginFields.clear();
    loginFields.push_back({350, 320, 400, 44, "Patient ID", "", false, false});
    loginFields.push_back({350, 408, 400, 44, "Password",   "", false, true});
}

// ============================================================
// ======= NEW: PATIENT CHOICE SCREEN =========================
// ============================================================
void drawPatientChoiceScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 310, 268, 480, 310, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 310, 268, 480, 5, CLINI_CYAN);

    drawText(win, font, "Patient", 550, 290, 24, CLINI_DARK, true, true);
    drawText(win, font, "Please choose an option to continue", 550, 326, 14, sf::Color(100,100,100), false, true);

    buttons.clear();
    buttons.push_back({370, 380, 360, 50, "Log In",   CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({370, 446, 360, 50, "Sign Up",  CLINI_CYAN, CLINI_WHITE});
    buttons.push_back({370, 518, 360, 40, "Back",     CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

void drawPatientLoginScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 270, 270, 560, 370, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 270, 270, 560, 5, CLINI_BLUE);
    drawText(win, font, "Patient Login", 550, 288, 21, CLINI_DARK, true, true);

    for (auto& f : loginFields) drawInputField(win, font, f);

    buttons.clear();
    buttons.push_back({350, 482, 400, 46, "Login",   CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({350, 540, 190, 38, "Back",    CLINI_GRAY, CLINI_WHITE});
    buttons.push_back({560, 540, 190, 38, "Sign Up", CLINI_CYAN, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

vector<InputField> signupFields;
void initSignupFields() {
    signupFields.clear();
    signupFields.push_back({200, 308, 320, 42, "Full Name",    "", false, false});
    signupFields.push_back({560, 308, 320, 42, "Age",          "", false, false});
    signupFields.push_back({200, 396, 320, 42, "Password",     "", false, true});
    signupFields.push_back({560, 396, 320, 42, "Disease",      "", false, false});
    signupFields.push_back({200, 484, 680, 42, "Phone Number", "", false, false});
}

void drawPatientSignupScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 170, 262, 760, 370, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 170, 262, 760, 5, CLINI_CYAN);
    drawText(win, font, "Create New Account", 550, 278, 21, CLINI_DARK, true, true);

    for (auto& f : signupFields) drawInputField(win, font, f);

    buttons.clear();
    buttons.push_back({200, 552, 320, 46, "Create Account", CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({560, 552, 320, 46, "Back",           CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

void drawPatientMenuScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons, int pid) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    string pname = "Patient";
    for (int i = 0; i < patientCount; i++)
        if (patients[i].id == pid) { pname = patients[i].name; break; }

    drawText(win, font, "Welcome, " + pname, 550, 268, 21, CLINI_DARK, true, true);
    drawText(win, font, "What would you like to do today?", 550, 300, 14, sf::Color(100,100,100), false, true);

    buttons.clear();
    float bx = 90, by = 345, bw = 195, bh = 52, gap = 24;
    buttons.push_back({bx,            by, bw, bh, "View Doctors",    CLINI_BLUE,            CLINI_WHITE});
    buttons.push_back({bx+bw+gap,     by, bw, bh, "Book Appointment", CLINI_CYAN,            CLINI_WHITE});
    buttons.push_back({bx+2*(bw+gap), by, bw, bh, "Cancel Appt",     CLINI_RED,             CLINI_WHITE});
    buttons.push_back({bx+3*(bw+gap), by, bw, bh, "View History",    CLINI_DARK,            CLINI_WHITE});
    buttons.push_back({bx+4*(bw+gap), by, bw, bh, "Find Appt",       sf::Color(103,58,183), CLINI_WHITE});
    buttons.push_back({420, 428, 260, 46, "Logout", CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

int doctorScrollOffset = 0;
void drawViewDoctorsScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawText(win, font, "Available Doctors", 550, 268, 21, CLINI_DARK, true, true);
    drawText(win, font, "Select a doctor and book your appointment instantly", 550, 296, 13, sf::Color(100,100,100), false, true);

    buttons.clear();

    if (doctorCount == 0) {
        drawText(win, font, "No Doctors Available", 550, 400, 19, CLINI_GRAY, false, true);
    } else {
        int start = doctorScrollOffset;
        int end = min(start + 3, doctorCount);
        for (int i = start; i < end; i++) {
            float cy = 318 + (i - start) * 112;
            bool co = doctors[i].checkedOut;

            // Card shadow + background
            drawRect(win, 83, cy + 3, 934, 100, CLINI_SHADOW);
            drawRect(win, 80, cy,     934, 100, CLINI_WHITE);
            drawRect(win, 80, cy,     6,   100, co ? CLINI_RED : CLINI_BLUE);

            // Doctor name + specialization
            drawText(win, font, doctors[i].name, 105, cy + 10, 17, CLINI_DARK_TEXT, true);
            drawText(win, font, "Specialist in: " + doctors[i].specialization, 105, cy + 34, 13, sf::Color(80,80,80));

            // Stars rating
            string stars = "";
            int fullStars = (int)doctors[i].rating;
            for (int s = 0; s < fullStars; s++) stars += "*";
            for (int s = fullStars; s < 5; s++) stars += "-";
            string ratingStr = stars + "  " + to_string(doctors[i].rating).substr(0,3) + "/5";
            drawText(win, font, ratingStr, 105, cy + 56, 13, sf::Color(255,160,0));

            // Info row: experience, bookings, fee
            string expStr  = "Experience: " + to_string(doctors[i].yearsOfExperience) + " yrs";
            string bkStr   = "Bookings: "   + to_string(doctors[i].totalBookings);
            string feeStr  = "Fee: "        + to_string((int)doctors[i].consultationFee) + " EGP";
            drawText(win, font, expStr,  105, cy + 76, 12, sf::Color(60,60,60));
            drawText(win, font, bkStr,   290, cy + 76, 12, sf::Color(60,60,60));
            drawText(win, font, feeStr,  470, cy + 76, 12, CLINI_BLUE, true);

            // Shift badge
            string shiftBadge = shiftName(doctors[i].shift) + " shift";
            drawRect(win, 640, cy + 14, 120, 26, CLINI_LIGHT_BG);
            drawText(win, font, shiftBadge, 700, cy + 18, 12, CLINI_DARK, false, true);

            // Book Now button
            if (!co && doctors[i].availableSlots > 0) {
                Button btn;
                btn.x = 780; btn.y = cy + 30; btn.w = 210; btn.h = 40;
                btn.label = "Book Now";
                btn.bgColor = CLINI_BLUE; btn.textColor = CLINI_WHITE;
                buttons.push_back(btn);
            } else {
                drawRect(win, 780, cy + 30, 210, 40, CLINI_GRAY);
                drawText(win, font, co ? "Doctor Left" : "No Slots", 885, cy + 42, 13, CLINI_WHITE, false, true);
                // push dummy
                Button dummy; dummy.x = -999; dummy.y = -999; dummy.w = 1; dummy.h = 1;
                dummy.label = "__unavail__"; dummy.bgColor = sf::Color::Transparent; dummy.textColor = sf::Color::Transparent;
                buttons.push_back(dummy);
            }
        }
    }

    if (doctorScrollOffset > 0)
        buttons.push_back({80,  668, 100, 36, "< Prev", CLINI_BLUE, CLINI_WHITE});
    if (doctorScrollOffset + 3 < doctorCount)
        buttons.push_back({920, 668, 100, 36, "Next >",  CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({425, 668, 250, 36, "Back", CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// =========== NEW: SELECT DOCTOR SCREEN ======================
// ============================================================
int selectDoctorScrollOffset = 0;

void drawSelectDoctorScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawText(win, font, "Step 1: Select a Doctor", 550, 268, 21, CLINI_DARK, true, true);
    drawText(win, font, "Click on a doctor to proceed to booking", 550, 296, 13, sf::Color(100,100,100), false, true);

    buttons.clear();

    int start = selectDoctorScrollOffset;
    int end   = min(start + 4, doctorCount);

    for (int i = start; i < end; i++) {
        float cy = 318 + (i - start) * 82;
        bool co = doctors[i].checkedOut;
        sf::Color cardBorder = co ? CLINI_RED : CLINI_BLUE;
        sf::Color textCol    = co ? CLINI_GRAY : CLINI_DARK_TEXT;

        drawRect(win, 83, cy + 3, 934, 72, CLINI_SHADOW);
        drawRect(win, 80, cy,     934, 72, CLINI_WHITE);
        drawRect(win, 80, cy,     5,   72, cardBorder);

        string info = doctors[i].name + "   |   " + doctors[i].specialization +
                      "   |   Shift: " + shiftName(doctors[i].shift) +
                      " (" + to_string(shiftStartHour(doctors[i].shift)) + ":00 - " +
                      to_string(shiftEndHour(doctors[i].shift)) + ":00)" +
                      "   |   Fee: " + to_string((int)doctors[i].consultationFee) + " EGP";
        string avail = co ? "Doctor has left" :
                       ("Available Slots: " + to_string(doctors[i].availableSlots));
        sf::Color availCol = co ? CLINI_RED : CLINI_GREEN;

        drawText(win, font, info,  100, cy + 12, 13, textCol, true);
        drawText(win, font, avail, 100, cy + 38, 13, availCol);

        if (!co && doctors[i].availableSlots > 0) {
            Button btn;
            btn.x = 870; btn.y = cy + 18; btn.w = 120; btn.h = 34;
            btn.label = "Select";
            btn.bgColor = CLINI_BLUE; btn.textColor = CLINI_WHITE;
            buttons.push_back(btn);
        } else {
            // placeholder so button index still aligns – draw a disabled visual
            drawRect(win, 870, cy + 18, 120, 34, CLINI_GRAY);
            drawText(win, font, co ? "Left" : "Full", 930, cy + 26, 14, CLINI_WHITE, false, true);
        }
    }

    if (selectDoctorScrollOffset > 0)
        buttons.push_back({80,  668, 100, 36, "< Prev", CLINI_BLUE, CLINI_WHITE});
    if (selectDoctorScrollOffset + 4 < doctorCount)
        buttons.push_back({920, 668, 100, 36, "Next >",  CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({425, 668, 250, 36, "Back", CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// =========== NEW: BOOK SLOT (Visual Grid) SCREEN ============
// ============================================================
int  bookingSelectedDoctor = -1;   // index in doctors[]
int  bookingSelectedSlot   = -1;   // 0-5
string bookingDate = "";

vector<InputField> bookDateField;
void initBookDateField() {
    bookDateField.clear();
    bookDateField.push_back({320, 310, 460, 44, "Date (YYYY-MM-DD, e.g. 2026-05-10)", "", false, false});
}

void drawBookSlotScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawText(win, font, "Step 2: Choose a Time Slot", 550, 268, 21, CLINI_DARK, true, true);

    Doctor& doc = doctors[bookingSelectedDoctor];
    string docInfo = doc.name + "   |   " + shiftLabel(doc.shift) +
                     "   |   Fee: " + to_string((int)doc.consultationFee) + " EGP";
    drawText(win, font, docInfo, 550, 296, 13, sf::Color(80,80,80), false, true);

    // Date input
    for (auto& f : bookDateField) drawInputField(win, font, f);

    // Hint
    drawText(win, font, "Date must be 2026-05-10 or later.", 550, 364, 12, sf::Color(150,80,0), false, true);

    // Slot grid (6 slots)
    drawText(win, font, "Available (Green) / Booked (Red):", 550, 390, 14, CLINI_DARK_TEXT, false, true);

    buttons.clear();

    float gx = 80, gy = 415, gw = 152, gh = 80, gap = 12;
    for (int s = 0; s < 6; s++) {
        float sx = gx + s * (gw + gap);
        string slotTime = slotTimeStr(doc.shift, s) + " - " +
                          slotTimeStr(doc.shift, s + 1 <= 5 ? s + 1 : s); // display end
        // compute end display
        int endH = shiftStartHour(doc.shift) + s + 1;
        char endBuf[8]; snprintf(endBuf, sizeof(endBuf), "%02d:00", endH);
        slotTime = slotTimeStr(doc.shift, s) + "\n- " + string(endBuf);

        bool booked = (!bookingDate.empty() && isDateValid(bookingDate))
                      ? isSlotBooked(doc.id, bookingDate, doc.shift, s)
                      : false;

        sf::Color slotColor = booked ? CLINI_RED : CLINI_GREEN;
        bool selected = (bookingSelectedSlot == s);

        drawRect(win, sx, gy, gw, gh, selected ? CLINI_DARK : slotColor,
                 selected ? CLINI_BLUE : sf::Color::Transparent, selected ? 3 : 0);

        string label1 = slotTimeStr(doc.shift, s);
        char endH2[8]; snprintf(endH2, sizeof(endH2), "%02d:00", shiftStartHour(doc.shift) + s + 1);
        string label2 = string("- ") + endH2;
        string label3 = booked ? "Booked" : "Available";

        drawText(win, font, label1, sx + gw / 2.f, gy + 10, 14, CLINI_WHITE, true, true);
        drawText(win, font, label2, sx + gw / 2.f, gy + 30, 14, CLINI_WHITE, false, true);
        drawText(win, font, label3, sx + gw / 2.f, gy + 52, 12, CLINI_WHITE, false, true);

        if (!booked) {
            Button btn;
            btn.x = sx; btn.y = gy; btn.w = gw; btn.h = gh;
            btn.label = ""; // invisible click zone
            btn.bgColor = sf::Color::Transparent;
            btn.textColor = sf::Color::Transparent;
            buttons.push_back(btn);
        } else {
            // push dummy so indices match
            Button dummy;
            dummy.x = -999; dummy.y = -999; dummy.w = 1; dummy.h = 1;
            dummy.label = "__booked__";
            dummy.bgColor = sf::Color::Transparent;
            dummy.textColor = sf::Color::Transparent;
            buttons.push_back(dummy);
        }
    }

    // Selected slot info
    if (bookingSelectedSlot >= 0) {
        char endH3[8]; snprintf(endH3, sizeof(endH3), "%02d:00", shiftStartHour(doc.shift) + bookingSelectedSlot + 1);
        string sel = "Selected: " + slotTimeStr(doc.shift, bookingSelectedSlot) + " - " + endH3;
        drawText(win, font, sel, 550, 510, 15, CLINI_DARK, true, true);
    }

    // Confirm + Back
    buttons.push_back({320, 555, 220, 44, "Confirm Booking", CLINI_BLUE, CLINI_WHITE}); // index 6
    buttons.push_back({560, 555, 220, 44, "Back",            CLINI_GRAY, CLINI_WHITE}); // index 7

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ========= Cancel / History / Find (unchanged) ==============
// ============================================================

vector<InputField> cancelFields;
void initCancelFields() {
    cancelFields.clear();
    cancelFields.push_back({320, 378, 460, 44, "Appointment ID to Cancel", "", false, false});
}

void drawCancelScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 280, 308, 540, 252, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 280, 308, 540, 5, CLINI_RED);
    drawText(win, font, "Cancel Appointment", 550, 324, 20, CLINI_DARK, true, true);

    for (auto& f : cancelFields) drawInputField(win, font, f);

    buttons.clear();
    buttons.push_back({320, 464, 220, 44, "Cancel Appt", CLINI_RED,  CLINI_WHITE});
    buttons.push_back({560, 464, 220, 44, "Back",        CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// For rate doctor screen
int ratingTargetDocID = -1;
int ratingSelectedStars = 0;

void drawViewHistoryScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawText(win, font, "My Appointments", 550, 268, 21, CLINI_DARK, true, true);

    int pid = loggedInPatientID;
    bool found = false;
    int row = 0;

    buttons.clear();

    for (int i = 0; i < appointmentCount && row < 5; i++) {
        if (appointments[i].patientID == pid) {
            float cy = 305 + row * 74;
            sf::Color sc = appointments[i].status == "Active" ? CLINI_GREEN : CLINI_RED;
            drawRect(win, 80, cy, 934, 64, CLINI_WHITE, CLINI_SHADOW, 1);
            drawRect(win, 80, cy, 5,   64, sc);

            // Find doctor name
            string docName = "Dr. ID " + to_string(appointments[i].doctorID);
            for (int d = 0; d < doctorCount; d++)
                if (doctors[d].id == appointments[i].doctorID) { docName = doctors[d].name; break; }

            string info = "Appt #" + to_string(appointments[i].appointmentID) +
                          "   " + docName +
                          "   Date: " + appointments[i].date +
                          "   Time: " + appointments[i].time;
            string cost = "Cost: " + to_string((int)appointments[i].totalCost) + " EGP  |  Status: " + appointments[i].status;

            drawText(win, font, info, 100, cy + 8,  13, CLINI_DARK_TEXT);
            drawText(win, font, cost, 100, cy + 32, 12, CLINI_BLUE);

            // Rate Doctor button
            Button rateBtn;
            rateBtn.x = 836; rateBtn.y = cy + 16; rateBtn.w = 160; rateBtn.h = 32;
            rateBtn.label = "Rate Doctor";
            rateBtn.bgColor = sf::Color(255,160,0); rateBtn.textColor = CLINI_WHITE;
            buttons.push_back(rateBtn);

            found = true; row++;
        }
    }

    if (!found)
        drawText(win, font, "No appointments found.", 550, 400, 17, CLINI_GRAY, false, true);

    buttons.push_back({425, 668, 250, 40, "Back", CLINI_GRAY, CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ============= NEW: RATE DOCTOR SCREEN ======================
// ============================================================
void drawRateDoctorScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 280, 268, 540, 360, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 280, 268, 540, 5, sf::Color(255,160,0));

    drawText(win, font, "Rate Your Doctor", 550, 286, 22, CLINI_DARK, true, true);

    string docName = "Doctor ID " + to_string(ratingTargetDocID);
    for (int d = 0; d < doctorCount; d++)
        if (doctors[d].id == ratingTargetDocID) { docName = doctors[d].name; break; }
    drawText(win, font, docName, 550, 322, 16, sf::Color(80,80,80), false, true);
    drawText(win, font, "Select a star rating (1 to 5):", 550, 354, 14, sf::Color(100,100,100), false, true);

    buttons.clear();

    // Star buttons
    float sx = 310, sy = 390, sw = 76, sh = 76, sgap = 10;
    for (int s = 1; s <= 5; s++) {
        float bx = sx + (s-1)*(sw+sgap);
        bool selected = (ratingSelectedStars >= s);
        drawRect(win, bx, sy, sw, sh, selected ? sf::Color(255,200,0) : sf::Color(230,230,230));
        drawText(win, font, to_string(s), bx + sw/2.f, sy + 26, 24, selected ? CLINI_WHITE : CLINI_GRAY, true, true);
        drawText(win, font, "★", bx + sw/2.f, sy + 52, 14, selected ? CLINI_WHITE : CLINI_GRAY, false, true);
        Button b; b.x = bx; b.y = sy; b.w = sw; b.h = sh;
        b.label = "__star" + to_string(s) + "__";
        b.bgColor = sf::Color::Transparent; b.textColor = sf::Color::Transparent;
        buttons.push_back(b);
    }

    if (ratingSelectedStars > 0) {
        string selText = "Your rating: " + to_string(ratingSelectedStars) + " / 5";
        drawText(win, font, selText, 550, 480, 15, sf::Color(255,140,0), true, true);
    }

    buttons.push_back({310, 510, 240, 46, "Submit Rating", sf::Color(255,160,0), CLINI_WHITE});
    buttons.push_back({570, 510, 240, 46, "Cancel",        CLINI_GRAY,           CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

vector<InputField> findFields;
string findResult = "";
void initFindFields() {
    findFields.clear();
    findFields.push_back({320, 358, 460, 44, "Appointment ID", "", false, false});
}

void drawFindAppointmentScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 280, 292, 540, 352, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 280, 292, 540, 5, sf::Color(103,58,183));
    drawText(win, font, "Find Appointment", 550, 308, 20, CLINI_DARK, true, true);

    for (auto& f : findFields) drawInputField(win, font, f);

    if (!findResult.empty())
        drawText(win, font, findResult, 295, 428, 12, CLINI_DARK_TEXT);

    buttons.clear();
    buttons.push_back({320, 555, 220, 44, "Search", sf::Color(103,58,183), CLINI_WHITE});
    buttons.push_back({560, 555, 220, 44, "Back",   CLINI_GRAY,            CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ========================= ADMIN ============================
// ============================================================

vector<InputField> adminLoginFields;
void initAdminLoginFields() {
    adminLoginFields.clear();
    adminLoginFields.push_back({350, 328, 400, 44, "Admin ID",  "", false, false});
    adminLoginFields.push_back({350, 416, 400, 44, "Password",  "", false, true});
}

void drawAdminLoginScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 270, 278, 560, 340, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 270, 278, 560, 5, CLINI_DARK);
    drawText(win, font, "Admin Login", 550, 294, 21, CLINI_DARK, true, true);

    for (auto& f : adminLoginFields) drawInputField(win, font, f);

    buttons.clear();
    buttons.push_back({350, 492, 400, 46, "Admin Login", CLINI_DARK, CLINI_WHITE});
    buttons.push_back({350, 550, 400, 38, "Back",        CLINI_GRAY, CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

void drawAdminMenuScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawText(win, font, "Admin Dashboard", 550, 268, 21, CLINI_DARK, true, true);

    drawRect(win, 80,  305, 200, 76, CLINI_WHITE, CLINI_SHADOW, 1);
    drawText(win, font, "Patients",  180, 316, 13, sf::Color(100,100,100), false, true);
    drawText(win, font, to_string(patientCount), 180, 340, 24, CLINI_BLUE, true, true);

    drawRect(win, 300, 305, 200, 76, CLINI_WHITE, CLINI_SHADOW, 1);
    drawText(win, font, "Doctors",   400, 316, 13, sf::Color(100,100,100), false, true);
    drawText(win, font, to_string(doctorCount), 400, 340, 24, CLINI_CYAN, true, true);

    drawRect(win, 520, 305, 200, 76, CLINI_WHITE, CLINI_SHADOW, 1);
    drawText(win, font, "Appointments", 620, 316, 13, sf::Color(100,100,100), false, true);
    drawText(win, font, to_string(appointmentCount), 620, 340, 24, CLINI_GREEN, true, true);

    buttons.clear();
    float bx = 80, by = 408, bw = 155, bh = 48, gap = 16;
    buttons.push_back({bx,              by, bw, bh, "Add Doctor",       CLINI_BLUE,               CLINI_WHITE});
    buttons.push_back({bx+bw+gap,       by, bw, bh, "Edit Slots",       CLINI_CYAN,               CLINI_WHITE});
    buttons.push_back({bx+2*(bw+gap),   by, bw, bh, "Remove Doctor",    CLINI_RED,                CLINI_WHITE});
    buttons.push_back({bx+3*(bw+gap),   by, bw, bh, "All Appointments", CLINI_DARK,               CLINI_WHITE});
    buttons.push_back({bx+4*(bw+gap),   by, bw, bh, "Reports",          CLINI_GREEN,              CLINI_WHITE});
    buttons.push_back({bx+5*(bw+gap),   by, bw, bh, "Doc Checkout",     sf::Color(120,40,180),    CLINI_WHITE});
    buttons.push_back({400, 480, 300, 44, "Logout", CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ======= NEW: ADD DOCTOR with Shift selection ===============
// ============================================================
int addDocShiftChoice = 0;  // 0/1/2

vector<InputField> addDocFields;
void initAddDocFields() {
    addDocFields.clear();
    addDocFields.push_back({200, 316, 320, 42, "Doctor Name",             "", false, false});
    addDocFields.push_back({560, 316, 320, 42, "Specialization",          "", false, false});
    addDocFields.push_back({200, 404, 320, 42, "Consultation Fee (EGP)",  "", false, false});
    addDocFields.push_back({560, 404, 320, 42, "Years of Experience",     "", false, false});
    addDocShiftChoice = 0;
}

void drawAddDoctorScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 170, 268, 760, 390, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 170, 268, 760, 5, CLINI_BLUE);
    drawText(win, font, "Add New Doctor", 550, 284, 20, CLINI_DARK, true, true);

    for (auto& f : addDocFields) drawInputField(win, font, f);

    // Shift picker label
    drawText(win, font, "Select Shift:", 200, 462, 14, sf::Color(100,100,100));

    buttons.clear();
    // Shift buttons (indices 0,1,2)
    sf::Color mBg = (addDocShiftChoice == 0) ? CLINI_BLUE  : CLINI_GRAY;
    sf::Color aBg = (addDocShiftChoice == 1) ? CLINI_CYAN  : CLINI_GRAY;
    sf::Color eBg = (addDocShiftChoice == 2) ? CLINI_DARK  : CLINI_GRAY;
    buttons.push_back({200, 482, 200, 40, "Morning  06-12", mBg, CLINI_WHITE});
    buttons.push_back({410, 482, 200, 40, "Afternoon 12-18", aBg, CLINI_WHITE});
    buttons.push_back({620, 482, 200, 40, "Evening  18-24", eBg, CLINI_WHITE});

    // Action buttons (indices 3,4)
    buttons.push_back({200, 542, 320, 44, "Add Doctor", CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({560, 542, 320, 44, "Back",       CLINI_GRAY, CLINI_WHITE});

    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ========== Edit Slots / Remove Doctor (unchanged) ==========
// ============================================================

vector<InputField> editSlotsFields;
void initEditSlotsFields() {
    editSlotsFields.clear();
    editSlotsFields.push_back({320, 358, 460, 44, "Doctor ID",           "", false, false});
    editSlotsFields.push_back({320, 446, 460, 44, "New Available Slots", "", false, false});
}

void drawEditSlotsScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 280, 296, 540, 304, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 280, 296, 540, 5, CLINI_CYAN);
    drawText(win, font, "Edit Doctor Slots", 550, 312, 20, CLINI_DARK, true, true);

    for (auto& f : editSlotsFields) drawInputField(win, font, f);

    buttons.clear();
    buttons.push_back({320, 524, 220, 44, "Update Slots", CLINI_CYAN, CLINI_WHITE});
    buttons.push_back({560, 524, 220, 44, "Back",         CLINI_GRAY, CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

vector<InputField> removeDocFields;
void initRemoveDocFields() {
    removeDocFields.clear();
    removeDocFields.push_back({320, 386, 460, 44, "Doctor ID to Remove", "", false, false});
}

void drawRemoveDoctorScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 280, 316, 540, 262, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 280, 316, 540, 5, CLINI_RED);
    drawText(win, font, "Remove Doctor", 550, 332, 20, CLINI_DARK, true, true);

    for (auto& f : removeDocFields) drawInputField(win, font, f);

    buttons.clear();
    buttons.push_back({320, 484, 220, 44, "Remove Doctor", CLINI_RED,  CLINI_WHITE});
    buttons.push_back({560, 484, 220, 44, "Back",          CLINI_GRAY, CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

int apptScrollOffset = 0;
void drawAdminViewAppointmentsScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawText(win, font, "All Appointments", 550, 268, 21, CLINI_DARK, true, true);

    if (appointmentCount == 0) {
        drawText(win, font, "No appointments yet.", 550, 400, 17, CLINI_GRAY, false, true);
    } else {
        drawRect(win, 80, 302, 940, 34, CLINI_BLUE);
        drawText(win, font, "ID    PatID  DocID  Date          Time      Status", 95, 309, 13, CLINI_WHITE, true);

        int start = apptScrollOffset;
        int end   = min(start + 6, appointmentCount);
        for (int i = start; i < end; i++) {
            float cy = 338 + (i - start) * 44;
            sf::Color rowBg = (i % 2 == 0) ? CLINI_WHITE : CLINI_LIGHT_BG;
            drawRect(win, 80, cy, 940, 40, rowBg);
            string line = to_string(appointments[i].appointmentID) + "      " +
                          to_string(appointments[i].patientID)     + "       " +
                          to_string(appointments[i].doctorID)      + "      " +
                          appointments[i].date + "   " + appointments[i].time;
            sf::Color sc = appointments[i].status == "Active" ? CLINI_GREEN : CLINI_RED;
            drawText(win, font, line, 95, cy + 12, 12, CLINI_DARK_TEXT);
            drawText(win, font, appointments[i].status, 880, cy + 12, 12, sc, true);
        }
    }

    buttons.clear();
    if (apptScrollOffset > 0)
        buttons.push_back({80,  668, 100, 36, "< Prev", CLINI_BLUE, CLINI_WHITE});
    if (apptScrollOffset + 6 < appointmentCount)
        buttons.push_back({920, 668, 100, 36, "Next >",  CLINI_BLUE, CLINI_WHITE});
    buttons.push_back({425, 668, 250, 36, "Back", CLINI_GRAY, CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

void drawAdminReportsScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 200, 268, 700, 370, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 200, 268, 700, 5, CLINI_GREEN);
    drawText(win, font, "System Reports", 550, 284, 21, CLINI_DARK, true, true);

    float totalRevenue = 0;
    int active = 0, cancelled = 0;
    for (int i = 0; i < appointmentCount; i++) {
        if (appointments[i].status == "Active") { active++; totalRevenue += appointments[i].totalCost; }
        else cancelled++;
    }

    auto drawStat = [&](const string& label, const string& val, float y, sf::Color vc) {
        drawRect(win, 228, y, 642, 52, CLINI_LIGHT_BG);
        drawText(win, font, label, 248, y + 16, 14, sf::Color(100,100,100));
        drawText(win, font, val,   820, y + 16, 17, vc, true);
    };

    drawStat("Total Patients Registered:", to_string(patientCount),           318, CLINI_BLUE);
    drawStat("Total Doctors Available:",   to_string(doctorCount),             380, CLINI_CYAN);
    drawStat("Active Appointments:",       to_string(active),                  442, CLINI_GREEN);
    drawStat("Cancelled Appointments:",    to_string(cancelled),               504, CLINI_RED);
    drawStat("Total Revenue:",             to_string((int)totalRevenue)+" EGP",566, CLINI_DARK);

    buttons.clear();
    buttons.push_back({390, 668, 320, 42, "Back to Admin Menu", CLINI_GRAY, CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ============ NEW: DOCTOR CHECKOUT SCREEN ===================
// ============================================================
vector<InputField> checkoutFields;
string checkoutInfo = "";

void initCheckoutFields() {
    checkoutFields.clear();
    checkoutFields.push_back({320, 340, 460, 44, "Doctor ID", "", false, false});
    checkoutInfo = "";
}

// Simulated current hour for demo: we use 7 to allow checkout after 1h from shift start
// In real use you'd call time() and extract hour
int getCurrentHour() {
    // Using a fixed demo hour of 8 (pretend it's 08:00 now).
    // Change this value to test different hours.
    return 8;
}

void drawDoctorCheckoutScreen(sf::RenderWindow& win, sf::Font& font, sf::Texture& logoTex, vector<Button>& buttons) {
    win.clear(CLINI_LIGHT_BG);
    drawNavbar(win, font, logoTex);
    drawHero(win, font);

    drawRect(win, 200, 272, 700, 400, CLINI_WHITE, CLINI_SHADOW, 1);
    drawRect(win, 200, 272, 700, 5, sf::Color(120,40,180));
    drawText(win, font, "Doctor Checkout", 550, 288, 20, CLINI_DARK, true, true);
    drawText(win, font, "Check out a doctor early or cancel remaining slots.", 550, 316, 13, sf::Color(100,100,100), false, true);

    for (auto& f : checkoutFields) drawInputField(win, font, f);

    if (!checkoutInfo.empty()) {
        // multi-line: split by \n
        float ly = 406;
        string line;
        istringstream ss(checkoutInfo);
        while (getline(ss, line)) {
            bool isErr = (line.find("ERROR") != string::npos || line.find("Cannot") != string::npos);
            drawText(win, font, line, 220, ly, 13, isErr ? CLINI_RED : CLINI_DARK_TEXT);
            ly += 22;
        }
    }

    buttons.clear();
    buttons.push_back({320, 588, 220, 44, "Checkout Doctor", sf::Color(120,40,180), CLINI_WHITE});
    buttons.push_back({560, 588, 220, 44, "Back",            CLINI_GRAY,            CLINI_WHITE});
    for (auto& b : buttons) drawButton(win, font, b);
    drawStatus(win, font);
}

// ============================================================
// ========================= MAIN =============================
// ============================================================
int main() {
    loadDataFromFile();

    sf::RenderWindow window(sf::VideoMode({1100u, 750u}), "CliniDo - Clinic Appointment System",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf"))
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
            return -1;

    sf::Texture logoTex;
    logoTex.loadFromFile("logo.png");

    vector<Button> buttons;
    InputField* focusedField = nullptr;

    initLoginFields();
    initSignupFields();
    initBookDateField();
    initCancelFields();
    initFindFields();
    initAdminLoginFields();
    initAddDocFields();
    initEditSlotsFields();
    initRemoveDocFields();
    initCheckoutFields();

    while (window.isOpen()) {
        sf::Vector2i mouse = sf::Mouse::getPosition(window);
        float mx = (float)mouse.x, my = (float)mouse.y;

        for (auto& b : buttons) b.isHovered = b.contains(mx, my);

        while (auto event = window.pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                saveDataToFile();
                window.close();
            }

            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    statusMessage = "";

                    auto checkFocus = [&](vector<InputField>& fields) {
                        focusedField = nullptr;
                        for (auto& f : fields) {
                            f.focused = f.contains(mx, my);
                            if (f.focused) focusedField = &f;
                        }
                    };

                    // -------- MAIN --------
                    if (currentScreen == SCREEN_MAIN) {
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) { initAdminLoginFields(); currentScreen = SCREEN_ADMIN_LOGIN; }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) { currentScreen = SCREEN_PATIENT_CHOICE; }
                        else if (buttons.size() > 2 && buttons[2].contains(mx, my)) { saveDataToFile(); window.close(); }
                    }

                    // -------- PATIENT CHOICE --------
                    else if (currentScreen == SCREEN_PATIENT_CHOICE) {
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) { initLoginFields(); currentScreen = SCREEN_PATIENT_LOGIN; }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) { initSignupFields(); currentScreen = SCREEN_PATIENT_SIGNUP; }
                        else if (buttons.size() > 2 && buttons[2].contains(mx, my)) currentScreen = SCREEN_MAIN;
                    }

                    // -------- PATIENT LOGIN --------
                    else if (currentScreen == SCREEN_PATIENT_LOGIN) {
                        checkFocus(loginFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            int id = atoi(loginFields[0].value.c_str());
                            string pass = loginFields[1].value;
                            bool found = false;
                            for (int i = 0; i < patientCount; i++)
                                if (patients[i].id == id && patients[i].password == pass) { found = true; loggedInPatientID = id; break; }
                            if (found) { setStatus("Login Successful!", false); currentScreen = SCREEN_PATIENT_MENU; }
                            else setStatus("Invalid ID or Password.", true);
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_PATIENT_CHOICE;
                        else if (buttons.size() > 2 && buttons[2].contains(mx, my)) { initSignupFields(); currentScreen = SCREEN_PATIENT_SIGNUP; }
                    }

                    // -------- PATIENT SIGNUP --------
                    else if (currentScreen == SCREEN_PATIENT_SIGNUP) {
                        checkFocus(signupFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            string name  = signupFields[0].value;
                            int age      = atoi(signupFields[1].value.c_str());
                            string pass  = signupFields[2].value;
                            string dis   = signupFields[3].value;
                            string phone = signupFields[4].value;
                            if (name.empty() || pass.empty()) setStatus("Name and Password required.", true);
                            else {
                                Patient p; p.id = patientCount + 1;
                                p.name = name; p.password = pass; p.disease = dis; p.age = age;
                                p.phones[0].number = phone; p.phoneCount = 1;
                                patients[patientCount++] = p;
                                setStatus("Account created! Your ID: " + to_string(p.id), false);
                                initSignupFields();
                            }
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_PATIENT_CHOICE;
                    }

                    // -------- PATIENT MENU --------
                    else if (currentScreen == SCREEN_PATIENT_MENU) {
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) { doctorScrollOffset = 0; currentScreen = SCREEN_VIEW_DOCTORS; }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) {
                            selectDoctorScrollOffset = 0;
                            currentScreen = SCREEN_SELECT_DOCTOR;
                        }
                        else if (buttons.size() > 2 && buttons[2].contains(mx, my)) { initCancelFields(); currentScreen = SCREEN_CANCEL_APPOINTMENT; }
                        else if (buttons.size() > 3 && buttons[3].contains(mx, my)) currentScreen = SCREEN_VIEW_HISTORY;
                        else if (buttons.size() > 4 && buttons[4].contains(mx, my)) { initFindFields(); findResult = ""; currentScreen = SCREEN_FIND_APPOINTMENT; }
                        else if (buttons.size() > 5 && buttons[5].contains(mx, my)) { loggedInPatientID = -1; currentScreen = SCREEN_MAIN; }
                    }

                    // -------- VIEW DOCTORS --------
                    else if (currentScreen == SCREEN_VIEW_DOCTORS) {
                        int start = doctorScrollOffset;
                        int end   = min(start + 3, doctorCount);
                        bool handled = false;
                        // First (end-start) buttons are "Book Now" for each card
                        for (int i = start; i < end && !handled; i++) {
                            int btnIdx = i - start;
                            if ((int)buttons.size() > btnIdx && buttons[btnIdx].label == "Book Now" && buttons[btnIdx].contains(mx, my)) {
                                bookingSelectedDoctor = i;
                                bookingSelectedSlot   = -1;
                                bookingDate           = "";
                                initBookDateField();
                                currentScreen = SCREEN_BOOK_SLOT;
                                handled = true;
                            }
                        }
                        if (!handled) {
                            for (auto& b : buttons) {
                                if (b.label == "< Prev" && b.contains(mx, my)) doctorScrollOffset -= 3;
                                if (b.label == "Next >"  && b.contains(mx, my)) doctorScrollOffset += 3;
                                if (b.label == "Back"    && b.contains(mx, my)) currentScreen = SCREEN_PATIENT_MENU;
                            }
                        }
                    }

                    // -------- SELECT DOCTOR --------
                    else if (currentScreen == SCREEN_SELECT_DOCTOR) {
                        // First 4 buttons (0-3) are the "Select" buttons for each visible doctor slot
                        int start = selectDoctorScrollOffset;
                        int end   = min(start + 4, doctorCount);
                        bool handled = false;
                        for (int i = start; i < end && !handled; i++) {
                            int btnIdx = i - start;
                            if ((int)buttons.size() > btnIdx && buttons[btnIdx].contains(mx, my)) {
                                if (!doctors[i].checkedOut && doctors[i].availableSlots > 0) {
                                    bookingSelectedDoctor = i;
                                    bookingSelectedSlot   = -1;
                                    bookingDate           = "";
                                    initBookDateField();
                                    currentScreen = SCREEN_BOOK_SLOT;
                                    handled = true;
                                }
                            }
                        }
                        if (!handled) {
                            for (auto& b : buttons) {
                                if (b.label == "< Prev" && b.contains(mx, my)) selectDoctorScrollOffset -= 4;
                                if (b.label == "Next >"  && b.contains(mx, my)) selectDoctorScrollOffset += 4;
                                if (b.label == "Back"    && b.contains(mx, my)) currentScreen = SCREEN_PATIENT_MENU;
                            }
                        }
                    }

                    // -------- BOOK SLOT --------
                    else if (currentScreen == SCREEN_BOOK_SLOT) {
                        checkFocus(bookDateField);
                        bookingDate = bookDateField[0].value;

                        // Buttons 0-5: slot selection
                        for (int s = 0; s < 6; s++) {
                            if ((int)buttons.size() > s && buttons[s].label != "__booked__" && buttons[s].contains(mx, my)) {
                                if (bookingDate.empty()) {
                                    setStatus("Please enter a date first.", true);
                                } else if (!isDateValid(bookingDate)) {
                                    setStatus("Invalid date! Must be 2026-05-10 or later (YYYY-MM-DD).", true);
                                } else if (isSlotBooked(doctors[bookingSelectedDoctor].id, bookingDate,
                                                         doctors[bookingSelectedDoctor].shift, s)) {
                                    setStatus("This slot is already booked!", true);
                                } else {
                                    bookingSelectedSlot = s;
                                }
                            }
                        }

                        // Button 6: Confirm
                        if ((int)buttons.size() > 6 && buttons[6].contains(mx, my)) {
                            bookingDate = bookDateField[0].value;
                            if (bookingDate.empty()) {
                                setStatus("Please enter a date.", true);
                            } else if (!isDateValid(bookingDate)) {
                                setStatus("Invalid date! Must be 2026-05-10 or later (YYYY-MM-DD).", true);
                            } else if (bookingSelectedSlot < 0) {
                                setStatus("Please select a time slot.", true);
                            } else {
                                Doctor& doc = doctors[bookingSelectedDoctor];
                                if (isSlotBooked(doc.id, bookingDate, doc.shift, bookingSelectedSlot)) {
                                    setStatus("That slot is already booked!", true);
                                } else if (doc.checkedOut) {
                                    setStatus("This doctor has already left for today.", true);
                                } else {
                                    string t = slotTimeStr(doc.shift, bookingSelectedSlot);
                                    appointments[appointmentCount] = {
                                        appointmentCount + 1, loggedInPatientID, doc.id,
                                        bookingDate, t, "Active", doc.consultationFee
                                    };
                                    appointmentCount++;
                                    doc.availableSlots--;
                                    doc.totalBookings++;
                                    setStatus("Booked! Appt ID: " + to_string(appointmentCount) +
                                              "  Time: " + t, false);
                                    bookingSelectedSlot = -1;
                                    bookingDate = "";
                                    initBookDateField();
                                }
                            }
                        }

                        // Button 7: Back
                        if ((int)buttons.size() > 7 && buttons[7].contains(mx, my)) {
                            currentScreen = SCREEN_SELECT_DOCTOR;
                        }
                    }

                    // -------- CANCEL APPOINTMENT --------
                    else if (currentScreen == SCREEN_CANCEL_APPOINTMENT) {
                        checkFocus(cancelFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            int aid = atoi(cancelFields[0].value.c_str());
                            int idx = findAppointmentIndex(aid);
                            if (idx == -1 || appointments[idx].status != "Active")
                                setStatus("Not Found or Already Cancelled.", true);
                            else {
                                for (int i = 0; i < doctorCount; i++)
                                    if (doctors[i].id == appointments[idx].doctorID) { doctors[i].availableSlots++; break; }
                                appointments[idx].status = "Cancelled";
                                setStatus("Cancelled Successfully.", false);
                                initCancelFields();
                            }
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_PATIENT_MENU;
                    }

                    // -------- VIEW HISTORY --------
                    else if (currentScreen == SCREEN_VIEW_HISTORY) {
                        // Buttons: 0..N-1 = Rate Doctor per appointment row, last = Back
                        int pid = loggedInPatientID;
                        int row = 0;
                        bool handled = false;
                        for (int i = 0; i < appointmentCount && row < 5 && !handled; i++) {
                            if (appointments[i].patientID == pid) {
                                if ((int)buttons.size() > row && buttons[row].label == "Rate Doctor" && buttons[row].contains(mx, my)) {
                                    ratingTargetDocID = appointments[i].doctorID;
                                    ratingSelectedStars = 0;
                                    currentScreen = SCREEN_RATE_DOCTOR;
                                    handled = true;
                                }
                                row++;
                            }
                        }
                        if (!handled) {
                            // last button = Back
                            if (!buttons.empty() && buttons.back().label == "Back" && buttons.back().contains(mx, my))
                                currentScreen = SCREEN_PATIENT_MENU;
                        }
                    }

                    // -------- RATE DOCTOR --------
                    else if (currentScreen == SCREEN_RATE_DOCTOR) {
                        // Star buttons: indices 0-4 (labels "__star1__" .. "__star5__")
                        for (int s = 1; s <= 5; s++) {
                            int idx = s - 1;
                            if ((int)buttons.size() > idx && buttons[idx].label == "__star" + to_string(s) + "__" && buttons[idx].contains(mx, my))
                                ratingSelectedStars = s;
                        }
                        // Submit: index 5
                        if ((int)buttons.size() > 5 && buttons[5].contains(mx, my)) {
                            if (ratingSelectedStars == 0) {
                                setStatus("Please select a star rating first.", true);
                            } else {
                                for (int d = 0; d < doctorCount; d++) {
                                    if (doctors[d].id == ratingTargetDocID) {
                                        float total = doctors[d].rating * doctors[d].ratingCount + ratingSelectedStars;
                                        doctors[d].ratingCount++;
                                        doctors[d].rating = total / doctors[d].ratingCount;
                                        break;
                                    }
                                }
                                setStatus("Thank you! Rating submitted successfully.", false);
                                currentScreen = SCREEN_VIEW_HISTORY;
                            }
                        }
                        // Cancel: index 6
                        else if ((int)buttons.size() > 6 && buttons[6].contains(mx, my))
                            currentScreen = SCREEN_VIEW_HISTORY;
                    }

                    // -------- FIND APPOINTMENT --------
                    else if (currentScreen == SCREEN_FIND_APPOINTMENT) {
                        checkFocus(findFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            int aid = atoi(findFields[0].value.c_str());
                            int idx = findAppointmentIndex(aid);
                            if (idx == -1) { findResult = "Appointment Not Found!"; setStatus("Not Found.", true); }
                            else {
                                findResult = "ID:" + to_string(appointments[idx].appointmentID) +
                                    " Pat:" + to_string(appointments[idx].patientID) +
                                    " Doc:" + to_string(appointments[idx].doctorID) +
                                    " Date:" + appointments[idx].date +
                                    " Time:" + appointments[idx].time +
                                    " Status:" + appointments[idx].status +
                                    " Cost:" + to_string((int)appointments[idx].totalCost) + "EGP";
                                setStatus("Found!", false);
                            }
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_PATIENT_MENU;
                    }

                    // -------- ADMIN LOGIN --------
                    else if (currentScreen == SCREEN_ADMIN_LOGIN) {
                        checkFocus(adminLoginFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            int id = atoi(adminLoginFields[0].value.c_str());
                            string pass = adminLoginFields[1].value;
                            bool found = false;
                            for (int i = 0; i < adminCount; i++)
                                if (admins[i].adminID == id && admins[i].password == pass) { found = true; break; }
                            if (found) { setStatus("Admin Login Successful!", false); currentScreen = SCREEN_ADMIN_MENU; }
                            else setStatus("Invalid Admin Credentials!", true);
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_MAIN;
                    }

                    // -------- ADMIN MENU --------
                    else if (currentScreen == SCREEN_ADMIN_MENU) {
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) { initAddDocFields(); currentScreen = SCREEN_ADMIN_ADD_DOCTOR; }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) { initEditSlotsFields(); currentScreen = SCREEN_ADMIN_EDIT_SLOTS; }
                        else if (buttons.size() > 2 && buttons[2].contains(mx, my)) { initRemoveDocFields(); currentScreen = SCREEN_ADMIN_REMOVE_DOCTOR; }
                        else if (buttons.size() > 3 && buttons[3].contains(mx, my)) { apptScrollOffset = 0; currentScreen = SCREEN_ADMIN_VIEW_APPOINTMENTS; }
                        else if (buttons.size() > 4 && buttons[4].contains(mx, my)) currentScreen = SCREEN_ADMIN_REPORTS;
                        else if (buttons.size() > 5 && buttons[5].contains(mx, my)) { initCheckoutFields(); currentScreen = SCREEN_ADMIN_DOCTOR_CHECKOUT; }
                        else if (buttons.size() > 6 && buttons[6].contains(mx, my)) currentScreen = SCREEN_MAIN;
                    }

                    // -------- ADMIN ADD DOCTOR --------
                    else if (currentScreen == SCREEN_ADMIN_ADD_DOCTOR) {
                        checkFocus(addDocFields);
                        // Shift picker: buttons 0,1,2
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) addDocShiftChoice = 0;
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) addDocShiftChoice = 1;
                        else if (buttons.size() > 2 && buttons[2].contains(mx, my)) addDocShiftChoice = 2;
                        // Add button: index 3
                        else if (buttons.size() > 3 && buttons[3].contains(mx, my)) {
                            string name = addDocFields[0].value;
                            string spec = addDocFields[1].value;
                            float fee   = atof(addDocFields[2].value.c_str());
                            int yexp    = atoi(addDocFields[3].value.c_str());
                            if (name.empty() || spec.empty()) setStatus("Name and Specialization required.", true);
                            else {
                                Doctor d;
                                d.id = doctorCount + 1;
                                d.name = name; d.specialization = spec;
                                d.slots = 6; d.availableSlots = 6;
                                d.consultationFee = fee;
                                d.shift = addDocShiftChoice;
                                d.checkInHour = -1;
                                d.checkedOut = false;
                                d.yearsOfExperience = yexp;
                                d.rating = 0.0f;
                                d.ratingCount = 0;
                                d.totalBookings = 0;
                                doctors[doctorCount++] = d;
                                setStatus("Doctor added! ID:" + to_string(d.id) +
                                          "  Shift: " + shiftName(d.shift), false);
                                initAddDocFields();
                            }
                        }
                        // Back: index 4
                        else if (buttons.size() > 4 && buttons[4].contains(mx, my)) currentScreen = SCREEN_ADMIN_MENU;
                    }

                    // -------- ADMIN EDIT SLOTS --------
                    else if (currentScreen == SCREEN_ADMIN_EDIT_SLOTS) {
                        checkFocus(editSlotsFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            int id    = atoi(editSlotsFields[0].value.c_str());
                            int slots = atoi(editSlotsFields[1].value.c_str());
                            bool found = false;
                            for (int i = 0; i < doctorCount; i++)
                                if (doctors[i].id == id) { doctors[i].availableSlots = slots; doctors[i].slots = slots; found = true; break; }
                            if (found) { setStatus("Slots updated.", false); initEditSlotsFields(); }
                            else setStatus("Doctor Not Found.", true);
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_ADMIN_MENU;
                    }

                    // -------- ADMIN REMOVE DOCTOR --------
                    else if (currentScreen == SCREEN_ADMIN_REMOVE_DOCTOR) {
                        checkFocus(removeDocFields);
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            int id = atoi(removeDocFields[0].value.c_str());
                            bool found = false;
                            for (int i = 0; i < doctorCount; i++)
                                if (doctors[i].id == id) {
                                    for (int j = i; j < doctorCount - 1; j++) doctors[j] = doctors[j + 1];
                                    doctorCount--; found = true; break;
                                }
                            if (found) { setStatus("Doctor removed.", false); initRemoveDocFields(); }
                            else setStatus("Doctor Not Found.", true);
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) currentScreen = SCREEN_ADMIN_MENU;
                    }

                    // -------- ADMIN VIEW APPOINTMENTS --------
                    else if (currentScreen == SCREEN_ADMIN_VIEW_APPOINTMENTS) {
                        for (auto& b : buttons) {
                            if (b.label == "< Prev" && b.contains(mx, my)) apptScrollOffset -= 6;
                            if (b.label == "Next >"  && b.contains(mx, my)) apptScrollOffset += 6;
                            if (b.label == "Back"    && b.contains(mx, my)) currentScreen = SCREEN_ADMIN_MENU;
                        }
                    }

                    // -------- ADMIN REPORTS --------
                    else if (currentScreen == SCREEN_ADMIN_REPORTS) {
                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) currentScreen = SCREEN_ADMIN_MENU;
                    }

                    // -------- ADMIN DOCTOR CHECKOUT --------
                    else if (currentScreen == SCREEN_ADMIN_DOCTOR_CHECKOUT) {
                        checkFocus(checkoutFields);

                        if (buttons.size() > 0 && buttons[0].contains(mx, my)) {
                            // "Checkout Doctor" pressed
                            int docID = atoi(checkoutFields[0].value.c_str());
                            int dIdx  = -1;
                            for (int i = 0; i < doctorCount; i++)
                                if (doctors[i].id == docID) { dIdx = i; break; }

                            if (dIdx == -1) {
                                checkoutInfo = "ERROR: Doctor ID not found.";
                                setStatus("Doctor not found.", true);
                            } else {
                                Doctor& doc = doctors[dIdx];

                                if (doc.checkedOut) {
                                    checkoutInfo = "ERROR: Doctor already checked out.";
                                    setStatus("Already checked out.", true);
                                } else {
                                    // Mark check-in if not done yet (first time admin interacts)
                                    if (doc.checkInHour < 0)
                                        doc.checkInHour = shiftStartHour(doc.shift);

                                    int now = getCurrentHour();
                                    int hoursWorked = now - doc.checkInHour;

                                    // Rule 1: must have worked at least 1 hour
                                    if (hoursWorked < 1) {
                                        checkoutInfo = "Cannot checkout: Doctor has not completed\n"
                                                       "1 hour yet. Hours worked: " +
                                                       to_string(hoursWorked) + "h\n"
                                                       "Shift: " + shiftLabel(doc.shift);
                                        setStatus("Cannot checkout yet.", true);
                                    } else {
                                        // Rule 2: no active appointments remaining in shift
                                        // (appointments in remaining time slots from 'now' onwards)
                                        int nowSlotIdx = now - shiftStartHour(doc.shift);
                                        // slots that haven't happened yet = nowSlotIdx to 5
                                        // We look for active appointments whose time >= now
                                        bool hasUpcoming = false;
                                        string blockedList = "";
                                        for (int i = 0; i < appointmentCount; i++) {
                                            if (appointments[i].doctorID == docID &&
                                                appointments[i].status == "Active") {
                                                // parse hour from time string "HH:00"
                                                int apptH = atoi(appointments[i].time.substr(0,2).c_str());
                                                if (apptH >= now) {
                                                    hasUpcoming = true;
                                                    blockedList += "  Appt #" +
                                                        to_string(appointments[i].appointmentID) +
                                                        " at " + appointments[i].time +
                                                        " (Pat ID:" + to_string(appointments[i].patientID) + ")\n";
                                                }
                                            }
                                        }

                                        if (hasUpcoming) {
                                            checkoutInfo = "Cannot checkout: Active upcoming appointments:\n" +
                                                           blockedList +
                                                           "Cancel these first, then retry.";
                                            setStatus("Upcoming appointments exist.", true);
                                        } else {
                                            // All clear: check out the doctor
                                            doc.checkedOut = true;
                                            doc.availableSlots = 0;

                                            // Cancel any remaining future slots (already no active ones,
                                            // but mark doc unavailable)
                                            checkoutInfo = "Doctor " + doc.name + " checked out.\n"
                                                           "Shift: " + shiftLabel(doc.shift) + "\n"
                                                           "Hours worked: " + to_string(hoursWorked) + "h\n"
                                                           "New bookings for this doctor are now blocked.";
                                            setStatus("Doctor checked out successfully.", false);
                                        }
                                    }
                                }
                            }
                        }
                        else if (buttons.size() > 1 && buttons[1].contains(mx, my)) {
                            currentScreen = SCREEN_ADMIN_MENU;
                        }
                    }
                }
            }

            if (const auto* textEv = event->getIf<sf::Event::TextEntered>()) {
                if (focusedField) {
                    uint32_t c = textEv->unicode;
                    if (c == 8) {
                        if (!focusedField->value.empty()) focusedField->value.pop_back();
                    } else if (c >= 32 && c < 128 && focusedField->value.size() < 40) {
                        focusedField->value += static_cast<char>(c);
                    }
                    // keep bookingDate in sync
                    if (currentScreen == SCREEN_BOOK_SLOT)
                        bookingDate = bookDateField[0].value;
                }
            }
        }

        switch (currentScreen) {
            case SCREEN_MAIN:                      drawMainScreen(window, font, logoTex, buttons); break;
            case SCREEN_PATIENT_CHOICE:            drawPatientChoiceScreen(window, font, logoTex, buttons); break;
            case SCREEN_PATIENT_LOGIN:             drawPatientLoginScreen(window, font, logoTex, buttons); break;
            case SCREEN_PATIENT_SIGNUP:            drawPatientSignupScreen(window, font, logoTex, buttons); break;
            case SCREEN_PATIENT_MENU:              drawPatientMenuScreen(window, font, logoTex, buttons, loggedInPatientID); break;
            case SCREEN_VIEW_DOCTORS:              drawViewDoctorsScreen(window, font, logoTex, buttons); break;
            case SCREEN_SELECT_DOCTOR:             drawSelectDoctorScreen(window, font, logoTex, buttons); break;
            case SCREEN_BOOK_SLOT:                 drawBookSlotScreen(window, font, logoTex, buttons); break;
            case SCREEN_CANCEL_APPOINTMENT:        drawCancelScreen(window, font, logoTex, buttons); break;
            case SCREEN_VIEW_HISTORY:              drawViewHistoryScreen(window, font, logoTex, buttons); break;
            case SCREEN_RATE_DOCTOR:               drawRateDoctorScreen(window, font, logoTex, buttons); break;
            case SCREEN_FIND_APPOINTMENT:          drawFindAppointmentScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_LOGIN:               drawAdminLoginScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_MENU:                drawAdminMenuScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_ADD_DOCTOR:          drawAddDoctorScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_EDIT_SLOTS:          drawEditSlotsScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_REMOVE_DOCTOR:       drawRemoveDoctorScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_VIEW_APPOINTMENTS:   drawAdminViewAppointmentsScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_REPORTS:             drawAdminReportsScreen(window, font, logoTex, buttons); break;
            case SCREEN_ADMIN_DOCTOR_CHECKOUT:     drawDoctorCheckoutScreen(window, font, logoTex, buttons); break;
        }

        window.display();
    }

    return 0;
}
