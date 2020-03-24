using System.Collections.Generic;
using System.Linq;
using Microsoft.AspNetCore.Mvc;
using System;

namespace Pyroserver.Controllers{

public class CollectDataModel{ // model pobierania danych z pirometru
    public int PyrometerId { get; set; }
    public string MaterialName { get; set; }
    public float MaterialEmissivity { get; set; }
    public float ObjectTemperature { get; set; }
    public float AmbientTemperature { get; set; }
};

public class UndescribedDataModel : CollectDataModel{ // model zapisu pomiaru do bazy danych 
    public int Id { get; set; } // ID pomiaru
    public string MeasurementDate { get; set; } // date i czas pomiaru tworzy serwer
};

public class DescribedDataModel{ // model zapisu opisu do pomiaru
    public int Id { get; set; } // po ID
    public string Description { get; set; } // opis pomiaru
};

    [ApiController]
    [Route("api")]
    public class CollectDataController : ControllerBase{

        [HttpGet("/described")] // endpoint
        public List<Measurement> Described(){ // wysyla liste wszystkich opisanych pomiarow
            using var db = new DatabaseContext();
            var result = (from x in db.Measurements where x.Description != null select x).ToList();
            return result;
        }

        [HttpGet("/undescribed")]
        public List<UndescribedDataModel> Undescribed(){ // wysyla liste wszystkich nieopisanych pomiarow
            using var db = new DatabaseContext();
            var result = (from x in db.Measurements where x.Description == null 
            select new UndescribedDataModel{
                Id = x.Id,
                PyrometerId = x.PyrometerId,
                MaterialName = x.MaterialName,
                MaterialEmissivity = x.MaterialEmissivity,
                ObjectTemperature = x.ObjectTemperature,
                AmbientTemperature = x.AmbientTemperature,
                MeasurementDate = x.MeasurementDate
            }).ToList();
            return result;
        }

        [HttpPost("/collectdata")]
        public IActionResult CollectData(CollectDataModel input){ // zebranie danych z pirometru
            using var db = new DatabaseContext();
            
            if (input.PyrometerId > 0 && input.MaterialEmissivity > 0 && input.MaterialName != null){ // jesli id i emisyjnosc wieksza od 0 i nazwa materialu inna niz null
                db.Add(new Measurement(){  // dodanie rekordu do bazy danych
                    PyrometerId = input.PyrometerId,
                    MaterialName = input.MaterialName,
                    MaterialEmissivity = input.MaterialEmissivity,
                    ObjectTemperature = input.ObjectTemperature,
                    AmbientTemperature = input.AmbientTemperature,
                    MeasurementDate = DateTime.Now.ToString()
                });
                db.SaveChanges(); // zapis zmian
                return Ok();
            }
            return BadRequest();
        }

        [HttpPost("/describedata")]
        public IActionResult DescribeData(DescribedDataModel input){ // opisanie pomiaru
            using var db = new DatabaseContext();
            var record = db.Measurements.SingleOrDefault(x => x.Id == input.Id); // wyszukanie po ID pomiaru z bazy danych

            if (input.Id > 0 && input.Description != null && record.Description == null && record != null){ // jesli dane wejsciowe sa prawidlowe
                record.Description = input.Description; // dodanie opisu
                db.SaveChanges(); // zapis zmian
                return Ok();
            }
            return BadRequest();
        }
    }
}