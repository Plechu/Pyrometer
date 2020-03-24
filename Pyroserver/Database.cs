using Microsoft.EntityFrameworkCore;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;

namespace Pyroserver
{
    public class DatabaseContext : DbContext
    {
        public DbSet<Measurement> Measurements { get; set; }

        protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
        {
            optionsBuilder.UseSqlite("Data Source=database.db");
        }
    }

    public class Measurement
    {
        [Required]
        public int Id { get; set; }
        [Required]
        public int PyrometerId { get; set; }
        [Required]
        public string MaterialName { get; set; }
        [Required]
        public float MaterialEmissivity { get; set; }
        [Required]
        public float ObjectTemperature { get; set; }
        [Required]
        public float AmbientTemperature { get; set; }
        [Required]
        public string MeasurementDate { get; set; }
        public string Description { get; set; }
        
    }
}
