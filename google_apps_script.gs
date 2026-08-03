// Smart Attendance System - Google Apps Script backend
//
// Sheet layout expected:
//   "Students" sheet   -> Column A: UID, Column B: Name
//   "Attendance" sheet -> Column A: Date, Column B: Time, Column C: UID, Column D: Name

var SPREADSHEET_ID = "Enter your Spreadsheet ID here";
var API_KEY = "change-this-to-a-long-random-string"; // must match config.h on the device
var TIMEZONE = "Asia/Kolkata";

function doGet(e) {
  var ss = SpreadsheetApp.openById(SPREADSHEET_ID);
  var studentsSheet = ss.getSheetByName("Students");
  var attendanceSheet = ss.getSheetByName("Attendance");

  // --- Basic auth check ---
  if (!e.parameter.key || e.parameter.key !== API_KEY) {
    return ContentService.createTextOutput("UNAUTHORIZED");
  }

  var uid = (e.parameter.uid || "").toString().trim().toUpperCase();
  if (!uid) {
    return ContentService.createTextOutput("MISSING_UID");
  }

  // --- Look up the student's name by UID ---
  var name = lookupName(studentsSheet, uid);
  if (!name) {
    return ContentService.createTextOutput("UNKNOWN");
  }

  // --- Prevent duplicate attendance for the same UID on the same day ---
  var today = Utilities.formatDate(new Date(), TIMEZONE, "yyyy-MM-dd");
  if (alreadyMarkedToday(attendanceSheet, uid, today)) {
    return ContentService.createTextOutput("DUPLICATE");
  }

  // --- Log attendance ---
  var now = new Date();
  var time = Utilities.formatDate(now, TIMEZONE, "HH:mm:ss");
  var nextRow = attendanceSheet.getLastRow() + 1;
  attendanceSheet.getRange(nextRow, 1, 1, 4).setValues([[today, time, uid, name]]);

  return ContentService.createTextOutput("OK:" + name);
}

// Scans the Students sheet for a matching UID and returns the name, or null.
function lookupName(studentsSheet, uid) {
  var data = studentsSheet.getDataRange().getValues();
  for (var i = 0; i < data.length; i++) {
    var rowUID = (data[i][0] || "").toString().trim().toUpperCase();
    if (rowUID === uid) {
      return data[i][1];
    }
  }
  return null;
}

// Checks whether this UID already has an entry for today's date.
function alreadyMarkedToday(attendanceSheet, uid, today) {
  var data = attendanceSheet.getDataRange().getValues();
  for (var i = 0; i < data.length; i++) {
    var rowDate = data[i][0];
    var rowUID = (data[i][2] || "").toString().trim().toUpperCase();
    if (rowDate === today && rowUID === uid) {
      return true;
    }
  }
  return false;
}
