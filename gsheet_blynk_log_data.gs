function logAllSensorData() {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var authToken = "1l6iZj-qkTcO1kRM8tBLPsUFNBka--z4";
  var baseUrl = "http://iot.serangkota.go.id:8080/" + authToken + "/get/";
  
  var pins = ["V0", "V1", "V2", "V3", "V4", "V5", "V6", "V7", "V8", "V9", "V10", "V11"];
  
  try {
    var rowData = [new Date()]; // Start with timestamp
    
    // Fetch all sensor data
    for (var i = 0; i < pins.length; i++) {
      var url = baseUrl + pins[i];
      var response = UrlFetchApp.fetch(url);
      var content = response.getContentText().trim();
      
      var value;
      if (pins[i] === "V10" || pins[i] === "V11") {
        // Handle text data (fuzzy outputs)
        var match = content.match(/\["([^"]+)"\]/);
        value = match && match[1] ? match[1] : "NO_DATA";
      } else {
        // Handle numeric data
        var match = content.match(/\["([\d.-]+)"\]/);
        value = match && match[1] ? parseFloat(match[1]) : 0;
      }
      
      rowData.push(value);
    }
    
    // Extract values for calculations
    var voltage = rowData[1] || 0;
    var current = rowData[2] || 0;
    var power = rowData[3] || 0;
    var powerFactor = rowData[4] || 0;
    var energyWh = rowData[6] || 0;
    var reactivePower = rowData[8] || 0;
    
    // Calculate additional metrics
    // 1. Current per kW
    // Golongan R-1/ TR daya 2.200 VA, Rp 1.444,70 per kWh. https://www.cnbcindonesia.com/lifestyle/20251014072259-33-675505/tarif-listrik-terbaru-pln-per-kwh-berlaku-14-oktober-2025
    var currentPerKW = power > 0 ? Math.round((current / power * 1000) * 100) / 100 : 0;
    
    // 2. Power Quality Score
    var powerQualityScore = 100;
    if (powerFactor < 0.9) powerQualityScore -= (0.9 - powerFactor) * 50;
    if (reactivePower > 500) powerQualityScore -= Math.min(20, (reactivePower - 500) / 50);
    var voltageDeviation = Math.abs(220 - voltage);
    if (voltageDeviation > 10) powerQualityScore -= Math.min(15, (voltageDeviation - 10));
    powerQualityScore = Math.max(0, Math.min(100, Math.round(powerQualityScore)));
    
    // 3. Energy Cost
    var energyCost = "Rp " + Math.round((energyWh / 1000) * 1444.70);
    
    // 4. Voltage Stability
    var voltageStability = Math.round(Math.abs(220 - voltage) / 220 * 100 * 10) / 10;
    
    // Add calculated metrics to row
    rowData.push(currentPerKW);
    rowData.push(powerQualityScore);
    rowData.push(energyCost); 
    rowData.push(voltageStability);
    
    // Append the complete row to the sheet
    sheet.appendRow(rowData);
    
    Logger.log("Data logged successfully at: " + new Date());
    
  } catch (e) {
    Logger.log("Error: " + e.toString());
  }
}
