using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;

namespace Doomlike
{
    internal class Wall
    {
        public Vector2 start;
        public Vector2 end;
        public int adjacentSector;
        public bool isPortal;

        public Wall(Vector2 start, Vector2 end, bool isPortal, int adjacentSector)
        {
            this.start = start;
            this.end = end;
            this.isPortal = isPortal;
            this.adjacentSector = adjacentSector;
        }
    }
}
