# 🏥 CliniDo — Online Clinic Appointment System

> A fully-featured, GUI-based clinic management system built in **C++ with SFML 3**, inspired by real-world healthcare platforms.

---

## 📌 Overview

CliniDo is a desktop application that simulates a complete clinic management ecosystem. It provides separate portals for **Patients**, **Doctors**, and **Admins**, with a polished graphical interface built entirely from scratch using SFML 3 — no game engine, no UI framework.

This project was built as a personal initiative to apply **Object-Oriented Programming**, **file persistence**, and **GUI design** concepts in a real-world context.

---

## ✨ Features

### 🧑‍⚕️ Patient Portal
- Secure Sign Up & Login
- Browse available doctors (specialization, shift, fee, rating, experience)
- Visual time-slot grid for booking appointments
- Cancel appointments
- View appointment history
- Rate doctors (1–5 stars) after consultation

### 🔧 Admin Panel
- Add / Remove doctors
- Edit doctor slots and availability
- View all appointments across the system
- Generate clinic reports (total bookings, revenue)
- Doctor check-in / check-out management with business rules enforcement

### 🗂️ Data Persistence
- All data (patients, doctors, appointments) is saved and loaded from `.txt` files
- Session state survives application restarts

---

## 🛠️ Tech Stack

| Technology | Purpose |
|---|---|
| **C++17** | Core application logic |
| **SFML 3** | GUI rendering (graphics, window, events) |
| **File I/O (fstream)** | Data persistence |
| **Custom UI System** | Buttons, input fields, screens — built from scratch |

---

## 🏗️ Architecture

The application is structured around a **screen-based state machine** with 18 distinct screens:

```
SCREEN_MAIN → SCREEN_PATIENT_CHOICE → SCREEN_PATIENT_LOGIN / SIGNUP
                                    → SCREEN_PATIENT_MENU
                                        → SCREEN_VIEW_DOCTORS
                                        → SCREEN_SELECT_DOCTOR
                                        → SCREEN_BOOK_SLOT
                                        → SCREEN_RATE_DOCTOR
                                        → SCREEN_VIEW_HISTORY
                                        → SCREEN_CANCEL_APPOINTMENT

SCREEN_MAIN → SCREEN_ADMIN_LOGIN → SCREEN_ADMIN_MENU
                                    → SCREEN_ADMIN_ADD_DOCTOR
                                    → SCREEN_ADMIN_EDIT_SLOTS
                                    → SCREEN_ADMIN_REMOVE_DOCTOR
                                    → SCREEN_ADMIN_VIEW_APPOINTMENTS
                                    → SCREEN_ADMIN_REPORTS
                                    → SCREEN_ADMIN_DOCTOR_CHECKOUT
```

**Core Data Structures:**
- `Patient` — ID, name, password, medical info, phone numbers
- `Doctor` — ID, specialization, shift, fee, rating, experience, booking stats
- `Appointment` — patient/doctor link, date, time slot, status, cost
- `Admin` — credentials and management access

---

## ⚙️ How to Compile

**Prerequisites:**
- MinGW-w64 (MSYS2 UCRT64 recommended)
- SFML 3 installed via MSYS2

```bash
g++ clinic_gui_v3.cpp -o clinido.exe \
  -I"C:\msys64\ucrt64\include" \
  -L"C:\msys64\ucrt64\lib" \
  -lsfml-graphics -lsfml-window -lsfml-system
```

**Required files in the same directory:**
```
arial.ttf
logo.png
```

---

## 🚀 How to Run

```bash
./clinido.exe
```

**Default Admin credentials:**
- Username: `Admin`
- Password: `1234`

**Sample Patient accounts** are preloaded for testing.

---

## 📐 Design Decisions

- **No external UI libraries** — all buttons, input fields, and layouts are rendered manually with SFML primitives. This was a deliberate choice to deeply understand 2D rendering pipelines.
- **Shift-based scheduling** — doctors are assigned Morning / Afternoon / Evening shifts, and the booking grid reflects real-time slot availability within those windows.
- **Checkout business rules** — an admin cannot check out a doctor who has upcoming active appointments, enforcing data integrity at the application level.
- **Rating aggregation** — doctor ratings are computed as running averages and persist across sessions.

---

## 🔮 Planned Improvements

- [ ] Replace flat-file storage with SQLite database
- [ ] Add search and filter functionality for doctors
- [ ] SMS / email appointment reminders (via API integration)
- [ ] Dark mode UI theme
- [ ] Export reports to PDF

---

## 👨‍💻 Author

**Ahmed Mustafa**
Computer Science Student — Ain Shams University
Volunteer Leader & Digital Transformation Lead — Heliopolis Friends NGO

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?logo=linkedin)](https://www.linkedin.com/in/ahmed-mustafa-aa7920384)

---

> *"Build things that solve real problems — even if it's just one clinic at a time."*
