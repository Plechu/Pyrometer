using Microsoft.EntityFrameworkCore.Migrations;

namespace Pyroserver.Migrations
{
    public partial class FirstMig : Migration
    {
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "Measurements",
                columns: table => new
                {
                    Id = table.Column<int>(nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    PyrometerId = table.Column<int>(nullable: false),
                    MaterialName = table.Column<string>(nullable: false),
                    MaterialEmissivity = table.Column<float>(nullable: false),
                    ObjectTemperature = table.Column<float>(nullable: false),
                    AmbientTemperature = table.Column<float>(nullable: false),
                    MeasurementDate = table.Column<string>(nullable: false),
                    Description = table.Column<string>(nullable: true)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Measurements", x => x.Id);
                });
        }

        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Measurements");
        }
    }
}
