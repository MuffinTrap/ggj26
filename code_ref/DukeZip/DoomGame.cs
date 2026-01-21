using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Numerics;
using Raylib_cs;
using DukeMapReader;

namespace Doomlike
{
    internal class DoomGame
    {
        Player player;
        Map episode1_level1;
        Camera2D topdownCamera;
        Camera2D topdownRotatedCamera;
        Camera3D fpsCamera;
        Material wallMaterial;
        Material floorMaterial;
        float scaleDown = 20.0f;
        public void Run()
        {
            Init();
            Loop();
        }

        private void Init()
        {
            Raylib.InitWindow(800, 600, "Doomlike");
            player = new Player();
            episode1_level1 = new Map();
            MapReader.MapInfo duke = MapReader.ReadMapFromFile("Maps/duke_map.map");
            Console.WriteLine(duke);

            // The coordinate system is 0 - 65 000
            float heightScaleDown = 600.0f;
            Vector2 origo = new Vector2(Int16.MaxValue, Int16.MaxValue); // Origo at 32_000, 32_000
            player.position = new Vector2((duke.posx-origo.X)/scaleDown, (duke.posy-origo.Y)/scaleDown);


            Image wallImg = Raylib.GenImageChecked(16, 16, 4, 4, Color.DarkGray, Color.RayWhite);
            Image floorImg = Raylib.GenImageChecked(16, 16, 8, 8, Color.DarkPurple, Color.DarkBlue);
            Texture2D wallTex = Raylib.LoadTextureFromImage(wallImg);
            Texture2D floorTex = Raylib.LoadTextureFromImage(floorImg);
            Raylib.UnloadImage(wallImg);

            int si = 0;
            // Go through all sectors and create walls
            foreach (MapReader.SectorInfo Si in duke.sectors)
            {
                Sector s = new Sector(si);
                si++;
                for (int w = 0; w < Si.wallnum; w++)
                {
                    // Every two points is one wall
                    MapReader.WallInfo Wi = duke.walls[Si.wallptr + w];
                    
                        MapReader.WallInfo W2 = duke.walls[Wi.point2];
                        s.walls.Add(new Wall(
                        new Vector2(
                            (Wi.x - origo.X) / scaleDown,
                            (Wi.y - origo.Y) / scaleDown),
                        new Vector2(
                            (W2.x - origo.X) / scaleDown,
                            (W2.y - origo.Y) / scaleDown), 
                        Wi.nextsector != -1,
                        Wi.nextsector));
                    
                }
                s.GenerateMeshes(Si.floorz, Si.ceilingz, 1.0f/heightScaleDown);
                s.GenerateModels(wallTex, floorTex);
                episode1_level1.sectors.Add(s);
            }

            fpsCamera = new Camera3D();
            fpsCamera.Position = new Vector3(player.position.X, 3.0f, player.position.Y);
            fpsCamera.Target = fpsCamera.Position + new Vector3(0.0f, 0.0f, 1.0f);
            fpsCamera.Up = new Vector3(0.0f, 1.0f, 0.0f);
            fpsCamera.FovY = 45.0f;
            fpsCamera.Projection = CameraProjection.Perspective;

            topdownCamera = new Camera2D();
            topdownCamera.Target = player.position;
            topdownCamera.Offset = Raylib.GetScreenCenter();
            topdownCamera.Zoom = 1.0f;
            topdownCamera.Rotation = 0.0f;

            topdownRotatedCamera = topdownCamera;

            topdownCamera.Offset.X -= Raylib.GetScreenWidth() / 4;
            topdownRotatedCamera.Offset.X += Raylib.GetScreenWidth() / 4;
        }

        private void Loop()
        {
            Raylib.DisableCursor();
            //Raylib.SetMousePosition(Raylib.GetScreenWidth() / 2, Raylib.GetScreenHeight() / 2);
            while(Raylib.WindowShouldClose() == false)
            {
                Raylib.UpdateCamera(ref fpsCamera, CameraMode.FirstPerson);
                player.Update(1.0f/scaleDown);
                topdownCamera.Target = player.position;
                Raylib.BeginDrawing();
                Raylib.ClearBackground(Color.DarkBlue);
                
                //DrawFirstPerson();
                Raylib.BeginMode3D(fpsCamera);
                episode1_level1.DrawModels();
                Raylib.DrawGrid(10, 10.0f);
                Raylib.EndMode3D();

                // Draw unrotated top down to left side
                Raylib.BeginMode2D(topdownCamera);

                episode1_level1.DrawTopdown();
                player.DrawTopdown();
                Raylib.EndMode2D();

                // Draw rotated top down to right side
                Raylib.BeginMode2D(topdownRotatedCamera);

                episode1_level1.DrawTopdownRotated(player);
                player.DrawTopdownRotated(Vector2.Zero, 0);
                Raylib.EndMode2D();

                Raylib.DrawText($"Player {(int)player.position.X}, {(int)player.position.Y}", 10, 10, 16, Color.White);

                Raylib.EndDrawing();

            }
        }

        void DrawFirstPerson()
        {
            // Floor
            Raylib.DrawRectangleGradientV(0, Raylib.GetScreenHeight() / 2, Raylib.GetScreenWidth(), Raylib.GetScreenHeight() / 2, Color.DarkGray, Color.LightGray);

            // Ceiling
            Raylib.DrawRectangleGradientV(0, 0, Raylib.GetScreenWidth(), Raylib.GetScreenHeight() / 2, Color.Beige, Color.DarkBrown);

            // Transform points to 3D
            episode1_level1.DrawFirstPerson(player);
        }
    }
}
