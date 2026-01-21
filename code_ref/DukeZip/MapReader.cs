using System;
using System.Formats.Tar;
using System.Text;

namespace DukeMapReader
{
    /// <summary>
    /// This class can read Duke Nukem 3D maps.
    /// Documentation about format is here: https://moddingwiki.shikadi.net/wiki/MAP_Format_(Build)
    /// 
    /// </summary>
    public class MapReader
    {
        public struct MapInfo
        {
            public readonly Int32 mapversion;

            public readonly Int32 posx; ///< Player position X
            public readonly Int32 posy; ///< Player position Y
            public readonly Int32 posz; ///< Player position Z
            public readonly Int16 ang;  ///< Player starting angle
            public readonly Int16 cursectnum; ///< Starting section number

            // Past this point you could not simply fread() into the struct all at once.
            // You would need to read the array counts, malloc the proper size, assign the result to the correct pointer, and then fread into that.
            public readonly Int16 numsectors;
            public readonly SectorInfo[]? sectors;
            public readonly Int16 numwalls;
            public readonly WallInfo[] walls;
            public readonly Int16 numsprites;
            public readonly SpriteInfo[]? sprites;
            public MapInfo()
            {
                numsectors = 0;
                numsprites = 0;
                numwalls = 0;
                sectors = Array.Empty<SectorInfo>();
                walls = Array.Empty<WallInfo>();
                sprites = Array.Empty<SpriteInfo>();
            }
            public MapInfo(BinaryReader reader) : this()
            {
                mapversion = reader.ReadInt32();
                posx = reader.ReadInt32();
                posy = reader.ReadInt32();
                posz = reader.ReadInt32();

                ang = reader.ReadInt16();
                cursectnum = reader.ReadInt16();

                numsectors = reader.ReadInt16();
                sectors = new SectorInfo[numsectors];
                for (int s = 0; s < numsectors; s++)
                {
                    sectors[s] = new SectorInfo(reader);
                }
                numwalls = reader.ReadInt16();
                walls = new WallInfo[numwalls];
                for (int s = 0; s < numwalls; s++)
                {
                    walls[s] = new WallInfo(reader);
                }
                numsprites = reader.ReadInt16();
                sprites = new SpriteInfo[numsprites];
                for (int s = 0; s < numsprites; s++)
                {
                    sprites[s] = new SpriteInfo(reader);

                }
            }

            public override string ToString()
            {
                StringBuilder b = new StringBuilder();
                b.Append($"-- Duke map version: {mapversion}.--\n");
                b.Append($"Player start position: ({posx},{posy},{posz}) Angle: {ang}.\n");
                b.Append($"Starting sector: {cursectnum}.\n");
                b.Append($"Sectors: {numsectors}.\n");
                b.Append($"Walls:   {numwalls}.\n");
                b.Append($"Sprites: {numsprites}.\n");
                b.Append($"-- Sectors --\n");
                for (int s = 0; s < numsectors; s++)
                {
                    b.Append($"Sector : {s}\n");
                    SectorInfo si = sectors[s];
                    b.Append($"Floor Z: {si.floorz} Ceiling Z:{si.ceilingz}");
                    b.Append($"Walls:\t{si.wallnum}\n");
                    for (int w = 0; w < si.wallnum; w++)
                    {
                        int wallIndex = si.wallptr + w;
                        b.Append($"{wallIndex}:\t{walls[wallIndex].ToString()}\n");
                    }
                }
                return b.ToString();
            }
        }
        public struct SectorInfo
        {
            public readonly Int16 wallptr, wallnum; ///< Index of first wall and amount of walls in this sector
            public readonly Int32 ceilingz, floorz; ///< Z of ceiling and floor of first point
            public readonly UInt16 ceilingstat, floorstat; ///< Stats about ceiling and floor
            public readonly Int16 ceilingpicnum; ///< Texture of ceiling
            public readonly Int16 ceilingheinum; ///< Sloping angle 0:flat, 4096: 45 degrees
            public readonly SByte ceilingshade;
            public readonly Byte ceilingpal, ceilingxpanning, ceilingypanning; ///< Palette index and texture olic ffsets
            public readonly Int16 floorpicnum, floorheinum;
            public readonly SByte floorshade;
            public readonly Byte floorpal, floorxpanning, floorypanning;
            public readonly Byte visibility; ///< How distance affects shading 
            public readonly Byte filler; ///< Padding byte


            readonly UInt16 lotag, hitag; ///< Game specific info
            readonly Int16 extra;
            public SectorInfo(BinaryReader reader)
            {
                wallptr = reader.ReadInt16();
                wallnum = reader.ReadInt16();

                ceilingz = reader.ReadInt32();
                floorz = reader.ReadInt32();

                ceilingstat = reader.ReadUInt16();
                floorstat = reader.ReadUInt16();
                ceilingpicnum = reader.ReadInt16();
                ceilingheinum = reader.ReadInt16();

                ceilingshade = reader.ReadSByte();

                ceilingpal = reader.ReadByte();
                ceilingxpanning = reader.ReadByte();
                ceilingypanning = reader.ReadByte();

                floorpicnum = reader.ReadInt16();
                floorheinum = reader.ReadInt16();

                floorshade = reader.ReadSByte();

                floorpal = reader.ReadByte();
                floorxpanning = reader.ReadByte();
                floorypanning = reader.ReadByte();
                visibility = reader.ReadByte();
                filler = reader.ReadByte();

                lotag = reader.ReadUInt16();
                hitag = reader.ReadUInt16();

                extra = reader.ReadInt16();
            }
        }

        public struct WallInfo
        {
            public readonly Int32 x, y; ///< Coordinates of the left side. Right side is left side of next wall.
            public readonly Int16 point2; ///< Index of next wall in sector's walls.
            public readonly Int16 nextwall; ///< Index of wall on the other side or -1 if no sector there
            public readonly Int16 nextsector; ///< Index of sector on the other side or -1
            public readonly UInt16 cstat; ///< Stats about wall
            public readonly Int16 picnum, overpicnum;
            public readonly SByte shade;
            public readonly Byte pal, xrepeat, yrepeat, xpanning, ypanning;
            readonly UInt16 lotag, hitag;
            readonly Int16 extra;
            public WallInfo(BinaryReader reader)
            {
                x = reader.ReadInt32();
                y = reader.ReadInt32();
                point2 = reader.ReadInt16();
                nextwall = reader.ReadInt16();
                nextsector = reader.ReadInt16();

                cstat = reader.ReadUInt16();
                picnum = reader.ReadInt16();
                overpicnum = reader.ReadInt16();
                shade = reader.ReadSByte();

                pal = reader.ReadByte();
                xrepeat = reader.ReadByte();
                yrepeat = reader.ReadByte();
                xpanning = reader.ReadByte();
                ypanning = reader.ReadByte();

                lotag = reader.ReadUInt16();
                hitag = reader.ReadUInt16();
                extra = reader.ReadInt16();

            }
            public override string ToString()
            {
                
                return $"Wall ({x},{y}) Next: {point2} Other {nextwall}";
            }
        }

        public struct SpriteInfo
        {
            public readonly Int32 x, y, z; ///< Sprite position in map
            public readonly UInt16 cstat;
            public readonly Int16 picnum;
            public readonly SByte shade;
            public readonly Byte pal, clipdist, filler;
            public readonly Byte xrepeat, yrepeat;
            public readonly SByte xoffset, yoffset;
            public readonly Int16 sectnum, statnum;
            public readonly Int16 ang, owner, xvel, yvel, zvel;
            public readonly UInt16 lotag, hitag;
            public readonly Int16 extra;
            public SpriteInfo(BinaryReader reader)
            {
                x = reader.ReadInt32();
                y = reader.ReadInt32();
                z = reader.ReadInt32();

                cstat = reader.ReadUInt16();
                picnum = reader.ReadInt16();
                shade = reader.ReadSByte();

                pal = reader.ReadByte();
                clipdist = reader.ReadByte();
                filler = reader.ReadByte();

                xrepeat = reader.ReadByte();
                yrepeat = reader.ReadByte();

                xoffset = reader.ReadSByte();
                yoffset = reader.ReadSByte();

                sectnum = reader.ReadInt16();
                statnum = reader.ReadInt16();

                ang = reader.ReadInt16();
                owner = reader.ReadInt16();
                xvel = reader.ReadInt16();
                yvel = reader.ReadInt16();
                zvel = reader.ReadInt16();

                lotag = reader.ReadUInt16();
                hitag = reader.ReadUInt16();
                extra = reader.ReadInt16();
            }
      
        }

        public static MapInfo ReadMapFromFile(string filename)
        {
            MapInfo map = new MapInfo();
            if (File.Exists(filename))
            {
                using (var stream = File.Open(filename, FileMode.Open))
                {
                    using (BinaryReader mapReader = new BinaryReader(stream, System.Text.Encoding.ASCII))
                    {
                        map = new MapInfo(mapReader);
                    }
                }
            }

            return map;
        }
    }
}
