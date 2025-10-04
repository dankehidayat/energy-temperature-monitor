function logAllSensorData() {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var authToken = "insertyourblynktokenhere"; // Your Blynk token
  var baseUrl = "http://iot.serangkota.go.id:8080/" + authToken + "/get/";
  
  // Array of all pins (V0-V9)
  var pins = ["V0", "V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9"];
  
  try {
    var rowData = [new Date()]; // Start with timestamp
    
    // Fetch each pin value, clean the response, and add to rowData
    for (var i = 0; i < pins.length; i++) {
      var url = baseUrl + pins[i];
      var response = UrlFetchApp.fetch(url);
      var jsonString = response.getContentText().trim();
      
      // Extract the numeric value from `["0.061"]` → `0.061`
      var value = parseFloat(jsonString.match(/\["([\d.]+)"\]/)[1]);
      
      rowData.push(value);
    }
    
    // Append the new row to the sheet
    sheet.appendRow(rowData);
    
  } catch (e) {
    Logger.log("Error fetching data: " + e);
  }
}
