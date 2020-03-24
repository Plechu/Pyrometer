// Please see documentation at https://docs.microsoft.com/aspnet/core/client-side/bundling-and-minification
// for details on configuring this project to bundle and minify static web assets.

// Write your JavaScript code.

var measurements;

$(document).ready(function () {
    getDescribed();
});

function getDescribed() {
    $.get("/described", function (data) {
        $("#content").empty();
        data.forEach(element => {
            $("#content").append("<div class='card m-2'><div class='card-header'><b>Pomiar #<span class='measurementId'>" + element.id + "</span></b></div><div class='card-body'><p class='card-text'><span>Identyfikator pirometru: " + element.pyrometerId + "</span><br><span>Materiał: " + element.materialName + "</span><br><span>Emisyjność materiału: " + element.materialEmissivity + "</span><br><span>Temperatura obiektu: " + element.objectTemperature + "</span><br><span>Temperatura otoczenia: " + element.ambientTemperature + "</span><br><span>Pomiar wykonano: " + element.measurementDate + "</span><br><span>Opis: " + element.description + "</span></p></div ></div >");
        });
    });
}
function getUndescribed() {
    $.get("/undescribed", function (data) {
        measurements = data;
        $("#content").empty();
        data.forEach(element => {
            $("#content").append("<div class='card m-2'><div class='card-header'><b>Pomiar #<span class='measurementId'>" + element.id + "</span></b></div><div class='card-body'><p class='card-text'><span>Identyfikator pirometru: " + element.pyrometerId + "</span><br><span>Materiał: " + element.materialName + "</span><br><span>Emisyjność materiału: " + element.materialEmissivity + "</span><br><span>Temperatura obiektu: " + element.objectTemperature + "</span><br><span>Temperatura otoczenia: " + element.ambientTemperature + "</span><br><span>Pomiar wykonano: " + element.measurementDate + "</span><br></p><button type='button' class='btn btn-outline-secondary' data-toggle='modal' data-target='#staticBackdrop' onclick='getModalInfo(event)'>Uzupełnij opis</button></div></div>");
        });
    });
}
function getModalInfo(e) {
    var number = e.target.parentElement.parentElement.childNodes[0].childNodes[0].childNodes[1].textContent;
    var measurementInfo = measurements.find(x => x.id == number);
    
    $("#id").text(measurementInfo.id);
    $("#measurementDate").text(measurementInfo.measurementDate);
    $("#pyrometerId").text(measurementInfo.pyrometerId);
    $("#materialName").text(measurementInfo.materialName);
    $("#materialEmissivity").text(measurementInfo.materialEmissivity);
    $("#objectTemperature").text(measurementInfo.objectTemperature);
    $("#ambientTemperature").text(measurementInfo.ambientTemperature);
    $("#description").val('');
}

function setDescription() {
    var input = {
        "id": parseInt($("#id").text()),
        "description": $("#description").val()
    };
    
    $.ajax({
        url:"/describedata",
        type:"POST",
        data:JSON.stringify(input),
        contentType:"application/json; charset=utf-8",
        dataType:"json",
        success: function(data, status){
            console.log('status: ' + status + ', data: ' + data.responseData);
        }
      })
    $('#staticBackdrop').modal('hide')
    getUndescribed();
}