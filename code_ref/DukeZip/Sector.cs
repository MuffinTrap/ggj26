using Raylib_cs;
using System.Numerics;

namespace Doomlike
{
    /// <summary>
    /// Sector contains the points
    /// that define it and also information
    /// about vertical size and position
    /// </summary>
    internal class Sector
    {
        int index;
        public List<Wall> walls;
        public List<Mesh> wallMeshhes;
        Mesh floorMesh;
        public List<Model> wallModels;
        Model floorModel;
        public float floorHeight;
        public float ceilingHeight;

        static Color[] SectorColors = { Color.White, Color.Pink, Color.Lime, Color.SkyBlue, Color.Yellow, Color.Gold };
        static Color[] WallColors = { Color.DarkBlue, Color.DarkBrown, Color.DarkPurple, Color.DarkGray, Color.DarkGreen, Color.Orange, Color.Maroon};

        public Sector(int index)
        {
            this.index = index;
            walls = new List<Wall>(4);
            floorHeight = -10;
            ceilingHeight = 10;
        }

        public void GenerateMeshes(int floorZ, int ceilingZ, float heightScale)
        {
            wallMeshhes = new List<Mesh>();
            // Every two points is one wall
            for (int i = 0; i < walls.Count; i+=1)
            {
                Wall wl = walls[i];
                if (wl.isPortal)
                {
                    // Create wall that goes down or up to adjacent sector: Note! both sectors dont need to do this. Only lower one
                    // if this floor height is greater than adjacent: Greate flipped wall  
                    // if this floor height is less than adjacent: Greate wall in between: goes up

                    // Ceiling:
                    // If this ceiling is higher than adjacent: Greate wall in between: goes down
                    continue;
                }
                Mesh w = new Mesh(4, 2);
                w.AllocVertices();
                w.AllocIndices();
                w.AllocTexCoords();

                Span<Vector3> vertices = w.VerticesAs<Vector3>();
                vertices[0] = new Vector3(wl.start.X, floorZ* heightScale * -1.0f, wl.start.Y); // 0
                vertices[1] = new Vector3(wl.end.X, floorZ* heightScale * -1.0f, wl.end.Y); // 1
                vertices[2] = new Vector3(wl.end.X, ceilingZ* heightScale * -1.0f, wl.end.Y); // 2
                vertices[3] = new Vector3(wl.start.X, ceilingZ* heightScale * -1.0f, wl.start.Y); // 3

                Span<Vector2> texCoords = w.TexCoordsAs<Vector2>();
                texCoords[0] = new Vector2(0.0f, 0.0f);
                texCoords[1] = new Vector2(1.0f, 0.0f);
                texCoords[2] = new Vector2(1.0f, 1.0f);
                texCoords[3] = new Vector2(0.0f, 1.0f);
                
                    Console.WriteLine($"Wall vertices {vertices[0]}, {vertices[1]}, {vertices[2]}, {vertices[3]}");
                

                Span<ushort> indices = w.IndicesAs<ushort>();
                indices[0] = 0;
                indices[1] = 1;
                indices[2] = 2;

                indices[3] = 2;
                indices[4] = 3;
                indices[5] = 0;
                //Raylib.DrawLineV(points[i], points[i + 1], SectorColors[index % SectorColors.Length]);
                Raylib.UploadMesh(ref w, false); // Save to GPU memory
                wallMeshhes.Add(w);

            }
            // Generate floor mesh
            // Take every other point.Points are arranged clockwise
            // 
            {
                floorMesh = new Mesh(4, 2);
                floorMesh.AllocVertices();
                floorMesh.AllocIndices();
                Span<Vector3> vertices = floorMesh.VerticesAs<Vector3>();
                int i = 0;
                vertices[0] = new Vector3(walls[0].start.X, floorZ * heightScale * -1.0f, walls[0].start.Y); // 0
                vertices[1] = new Vector3(walls[0].end.X, floorZ * heightScale * -1.0f, walls[0].end.Y); // 1
                vertices[2] = new Vector3(walls[2].start.X, floorZ * heightScale * -1.0f, walls[2].start.Y); // 2
                vertices[3] = new Vector3(walls[2].end.X, floorZ * heightScale * -1.0f, walls[2].end.Y); // 3

                Span<ushort> indices = floorMesh.IndicesAs<ushort>();
                indices[0] = 0;
                indices[1] = 3;
                indices[2] = 2;

                indices[3] = 2;
                indices[4] = 1;
                indices[5] = 0;
                Raylib.UploadMesh(ref floorMesh, false);
            }

            //Raylib.
        }
        public void GenerateModels(Texture2D wallTexture, Texture2D floorTexture)
        {
            wallModels = new List<Model>();
            for(int i = 0; i < wallMeshhes.Count; i++)
            {
                Model wm = Raylib.LoadModelFromMesh(wallMeshhes[i]);
                Raylib.SetMaterialTexture(ref wm, 0, MaterialMapIndex.Albedo, ref wallTexture);
                wallModels.Add(wm);
                
            }
            floorModel = Raylib.LoadModelFromMesh(floorMesh);
            Raylib.SetMaterialTexture(ref floorModel, 0, MaterialMapIndex.Albedo, ref floorTexture);
        }

        /// <summary>
        /// Draws the sector from top down
        /// view as it is in the world
        /// </summary>
        public void DrawTopdown()
        {
            
            for (int i = 0; i < walls.Count; i++)
            {
                Raylib.DrawText(i.ToString(), (int)walls[i].start.X-6, (int)walls[i].start.Y-6, 16, Color.White);
                Raylib.DrawText(i.ToString(), (int)walls[i].end.X+6, (int)walls[i].end.Y+6, 16, Color.White);
                Raylib.DrawLineV(walls[i].start, walls[i].end, SectorColors[index% SectorColors.Length]);
                
            }

        }

        public void DrawModels(Vector3 center)
        {
            for (int i = 0; i < wallModels.Count; i++)
            {
                Raylib.DrawModel(wallModels[i], center, 1.0f, Color.White);
            }
            Raylib.DrawModel(floorModel, center, 1.0f, Color.White);

        }


        /// <summary>
        /// Draws the sector from top down
        /// view rotated and centered around player
        /// </summary>
        /// <param name="player">Player to rotate and center around</param>
        public void DrawTopdownRotated(Player player)
        {
           
            Vector2 A;
            Vector2 B;
            Matrix3x2 playerRotation = Matrix3x2.CreateRotation(-player.rotationRad);
            for (int i = 0; i < walls.Count; i++)
            {
                A = walls[i].start;
                B = walls[i].end;
                // Rotate around player
                A -= player.position;
                B -= player.position;


                A = Vector2.Transform(A, playerRotation);
                B = Vector2.Transform(B, playerRotation);
                Raylib.DrawLineV(A, B, SectorColors[index % SectorColors.Length]);
                

            }
        }

        /// <summary>
        /// If point is behind a player, this calculates
        /// a new point that is in the view
        /// This is used when one point of wall is behind player
        /// </summary>
        /// <param name="A">The point that is behind the player</param>
        /// <param name="B">The other point of the same wall</param>
        /// <param name="Az"></param>
        /// <param name="Bz"></param>
        /// <param name="clipZ"></param>
        /// <returns>Clipped version of A that is in front of player</returns>
        private Vector2 clipBehindPlayer(Vector2 A, Vector2 B, float Az, float Bz, out float clipZ)
        {
            float distanceA = A.Y;
            float distanceB = B.Y;
            float distanceDelta = distanceA - distanceB;
            if (MathF.Abs(distanceDelta) < 0.01f)
            {
                distanceDelta = 1.0f; // Avoid dividing by zero
            }
            float intoDepth = distanceA / distanceDelta;
            Vector2 clippedA = new Vector2(
                A.X + intoDepth * (B.X - A.X),
                A.Y + intoDepth * (B.Y - A.Y));
            clipZ = Az + intoDepth * (Bz - Az);
            if (MathF.Abs(clippedA.Y) < 0.01f) { clippedA.Y = 1; } // Don't return a point with depth of 0
            return clippedA;

        }

        /// <summary>
        /// Draws a given wall between points A and B
        /// rotated and centered around player, 
        /// rendered in 3D with the given color
        /// </summary>
        /// <param name="A">Point A of wall</param>
        /// <param name="B">Point B of wall</param>
        /// <param name="player">Player to center and rotate</param>
        /// <param name="wallColor">What color to use for the wall</param>
        private void DrawWall(Vector2 A, Vector2 B, Player player, Color wallColor)
        {
            int SW = Raylib.GetScreenWidth();
            int SH = Raylib.GetScreenHeight();
            // Rotate around player
            A -= player.position;
            B -= player.position;

            Matrix3x2 playerRotation = Matrix3x2.CreateRotation(-player.rotationRad);
            A = Vector2.Transform(A, playerRotation);
            B = Vector2.Transform(B, playerRotation);

            // Calculate viewport scaling 
            // and store height values to variables
            float Zbottom = floorHeight;
            float Ztop = ceilingHeight;
            float scaleX = -1 * Raylib.GetScreenHeight() / 2.0f * MathF.Sin(player.fovDeg * Raylib.DEG2RAD);
            float scaleY = scaleX;

            // Read depth from rotated points
            float depthA = A.Y;
            float depthB = B.Y;

            // If both points are behind player, wall can be skipped
            if (depthA < 1 && depthB < 1)
            {
                // Behind player
                return;
            }

            // If one wall is behind player, it is clipped
            float zOut = 0.0f;
            if (depthA < 1)
            {
                // A is behind player
                A = clipBehindPlayer(A, B, Zbottom, Zbottom, out zOut);
            }
            else if (depthB < 1)
            {
                B = clipBehindPlayer(B, A, Zbottom, Zbottom, out zOut);
            }
            // Refresh depths from clipped points
            depthA = A.Y;
            depthB = B.Y;

            // Calculate the points in view
            Vector2 perspectiveA = new Vector2(
                (A.X * scaleX) / depthA,
                Zbottom * scaleY / depthA);

            Vector2 perspectiveB = new Vector2(
                (B.X * scaleX) / depthB, 
                Zbottom * scaleY / depthB);

            Vector2 perspectiveAtop = new Vector2((A.X * scaleX) / depthA, Ztop * scaleY / depthA);
            Vector2 perspectiveBtop = new Vector2((B.X * scaleX) / depthB, Ztop * scaleY / depthB);

            // From now .Y is the Z(height) value

            // Transform to view space: center to screen
            perspectiveA += Raylib.GetScreenCenter();
            perspectiveB += Raylib.GetScreenCenter();
            perspectiveAtop += Raylib.GetScreenCenter();
            perspectiveBtop += Raylib.GetScreenCenter();

            // Calculate variables that are needed to 
            // fill in the vertical lines
            // The lines are drawn from left to right
            Vector2 startBottom;
            Vector2 startTop;
            Vector2 endBottom;
            Vector2 endTop;
            float deltaX = 0;
            float deltaY_top = 0;
            float deltaY_bottom = 0;

            // Swap points if necessary
            startBottom = perspectiveB;
            startTop = perspectiveBtop;
            endBottom = perspectiveA;
            endTop = perspectiveAtop;
            if (perspectiveA.X < perspectiveB.X)
            {
                startBottom = perspectiveA;
                startTop = perspectiveAtop;
                endBottom = perspectiveB;
                endTop = perspectiveBtop;
            }

            // Calculate changes in X and Y
            deltaX = endTop.X - startTop.X;
            deltaY_bottom = endBottom.Y - startBottom.Y;
            deltaY_top = endTop.Y - startTop.Y;

            // Calculate Y steps to take between
            // vertial lines

            float stepDeltaY_top = deltaY_top / deltaX;
            float stepDeltaY_bottom = deltaY_bottom / deltaX;
            float top_Y = startTop.Y;
            float bottom_Y = startBottom.Y;

            // Clip to Window
            // Does not work correctly
            /*
            if (startBottom.X < 1)
            {
                bottom_Y += startBottom.X * -1 * stepDeltaY_bottom;
                startBottom.X = 1;
            }
            if (endBottom.X > SW-1)
            {
                endBottom.X = SW-1;
            }
            if (startTop.X < 1)
            {
                top_Y += startTop.X * -1 * stepDeltaY_top;
                startTop.X = 1;
            }
            if (endTop.X > SW - 1)
            {
                endTop.X = SW - 1;
            }
            */

            // Start plotting the vertical walls
            // Clamp them inside window vertically
            int bottom_plot = 0;
            int top_plot = 0;
            for (int x = (int)startBottom.X; x < (int)endBottom.X; x++)
            {
                top_plot = (int)top_Y;
                bottom_plot = (int)bottom_Y;
                
                if (top_plot < 1) { top_plot = 1; }
                if (bottom_plot > SH - 1) { bottom_plot = SH - 1; }

                Raylib.DrawLine(x, top_plot, x, bottom_plot, wallColor);
                
                top_Y += stepDeltaY_top;
                bottom_Y += stepDeltaY_bottom;
            }

            // Draw Rectangles to the points
            Raylib.DrawRectangle((int)perspectiveA.X - 2, (int)perspectiveA.Y - 2, 4, 4, Color.White);
            Raylib.DrawRectangle((int)perspectiveAtop.X - 2, (int)perspectiveAtop.Y - 2, 4, 4, Color.Pink);

            Raylib.DrawRectangle((int)perspectiveB.X - 2, (int)perspectiveB.Y - 2, 4, 4, Color.Red);
            Raylib.DrawRectangle((int)perspectiveBtop.X - 2, (int)perspectiveBtop.Y - 2, 4, 4, Color.Red);


            // Draw lines on top and bottom line
            Raylib.DrawLineV(perspectiveA, perspectiveB, Color.Pink);
            Raylib.DrawLineV(perspectiveAtop, perspectiveBtop, Color.Pink);
        }

        /// <summary>
        /// Draws the sector from the view of the player
        /// </summary>
        /// <param name="player"></param>
        public void DrawFirstPerson(Player player)
        {
            int wallCounter = 0;
            Vector2 A;
            Vector2 B;
            for (int i = 0; i < walls.Count; i++)
            {
                A = walls[i].start;
                B = walls[i].end;
                DrawWall(A, B, player, WallColors[wallCounter]);
                wallCounter = (wallCounter + 1) % WallColors.Length;
            }
        }
    }
}
