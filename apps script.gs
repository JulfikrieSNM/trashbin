function doGet(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet()
                    .getSheetByName("Sheet1");

  var lastRow = sheet.getLastRow();

  if (lastRow < 2) {
    return ContentService
      .createTextOutput(JSON.stringify({
        error: "No data available"
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }

  var headers = sheet.getRange(1, 1, 1, sheet.getLastColumn())
                    .getValues()[0];

  var values = sheet.getRange(lastRow, 1, 1, sheet.getLastColumn())
                    .getValues()[0];

  var result = {};

  for (var i = 0; i < headers.length; i++) {
    result[headers[i]] = values[i];
  }

  return ContentService
    .createTextOutput(JSON.stringify(result))
    .setMimeType(ContentService.MimeType.JSON);
}
