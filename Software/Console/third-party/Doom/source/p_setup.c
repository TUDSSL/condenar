/**
 *  Doom Port to the nRF52840 by Nicola Wrachien (next-hack in the comments)
 *
 *  This port is based on the excellent doomhack's GBA Doom Port,
 *  with Kippykip additions.
 *  
 *  Several data structures and functions have been optimized 
 *  to fit in only 256kB RAM of nRF52840 (GBA has 384 kB RAM). 
 *  Z-Depth Light has been restored with almost no RAM consumption!
 *  Tons of speed optimizations have been done, and the game now
 *  runs extremely fast, despite the much higher 3D resolution with
 *  respect to GBA.
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright (C) 2021 Nicola Wrachien (next-hack in the comments)
 *  on nRF52840 port.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  Do all the WAD I/O, get map description,
 *  set up initial state and misc. LUTs.
 *
 * next-hack: heavily modified load functions to support caching of WAD-wise
 * and level-wise constant data to flash, saving tons of RAM.
 *-----------------------------------------------------------------------------*/

#include <math.h>

#include "doomstat.h"
#include "m_bbox.h"
#include "g_game.h"
#include "w_wad.h"
#include "r_main.h"
#include "r_things.h"
#include "p_maputl.h"
#include "p_map.h"
#include "p_setup.h"
#include "p_spec.h"
#include "p_tick.h"
#include "p_enemy.h"
#include "s_sound.h"
#include "lprintf.h" //jff 10/6/98 for debug outputs
#include "v_video.h"
#include "w_wad.h"

#include "global_data.h"
#include "r_sky.h"
#include "i_spi_support.h"
#include "Kernel_Draw.h"

extern patch_t * cachedColumnOffsetDataPatch;
//
// P_LoadVertexes
//
// killough 5/3/98: reformatted, cleaned up
//
static void P_LoadVertexes(int lump)
{
    // Determine number of lumps:
    //  total lump length / vertex record length.
    _g->numvertexes = W_LumpLength(lump) / sizeof(vertex_t);
    // Allocate zone memory for buffer.
    _g->vertexes = writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);
}

//
// P_LoadSegs
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSegs(int lump)
{
    _g->numsegs = W_LumpLength(lump) / sizeof(seg_t); 
    _g->segs = (const seg_t*) writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);

//    if (!_g->numsegs)
//      I_Error("P_LoadSegs: no segs in level");
}

//
// P_LoadSubsectors
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSubsectors(int lump, bool isNormalStartup)
{
    /* cph 2006/07/29 - make data a const mapsubsector_t *, so the loop below is simpler & gives no constness warnings */
    const mapsubsector_t *data;
    int i;

    _g->numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
  //  if(_g->subsectors!=NULL) Z_Free(_g->subsectors);
    if(isNormalStartup) _g->subsectors = Z_Calloc(_g->numsubsectors, sizeof(subsector_t), PU_LEVEL, 0);
    memset(_g->subsectors,0,_g->numsubsectors*sizeof(subsector_t));
    data = (const mapsubsector_t*) W_CacheLumpNum(lump);

    if ((!data) || (!_g->numsubsectors))
        I_Error("P_LoadSubsectors: no subsectors in level");

    spiFlashSetAddress((uint32_t) data);
    for (i = 0; i < _g->numsubsectors; i++)
    {
        mapsubsector_t subsector;
        spiFlashGetData(&subsector, sizeof(subsector));
        //_g->subsectors[i].numlines  = (unsigned short)SHORT(data[i].numsegs );
        //_g->subsectors[i].firstline = (unsigned short)SHORT(data[i].firstseg);
        _g->subsectors[i].numlines = (unsigned short) SHORT(subsector.numsegs);
        _g->subsectors[i].firstline = (unsigned short) SHORT(subsector.firstseg);
    }
}

//
// P_LoadSectors
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSectors(int lump, bool isNormalStartup)
{
    const byte *data; // cph - const*
    int i;

    _g->numsectors = W_LumpLength(lump) / sizeof(mapsector_t);

  //  if(_g->sectors!=NULL) Z_Free(_g->sectors);
    if(isNormalStartup) _g->sectors = Z_Calloc(_g->numsectors, sizeof(sector_t), PU_LEVEL, 0);
  //  memset(_g->sectors,0,_g->numsectors*sizeof(sector_t));
    data = W_CacheLumpNum(lump); // cph - wad lump handling updated

    spiFlashSetAddress((uint32_t) data);

    for (i = 0; i < _g->numsectors; i++)
    {
#if 0
      sector_t *ss = _g->sectors + i;

      const mapsector_t *ms = (const mapsector_t *) data + i;

      ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
      ss->floorpic = R_FlatNumForName(ms->floorpic);
      ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);

      ss->lightlevel = SHORT(ms->lightlevel);
      ss->special = SHORT(ms->special);
      ss->oldspecial = SHORT(ms->special);
      ss->tag = SHORT(ms->tag);

      ss->thinglist_sptr = 0;
      ss->touching_thinglist_sptr = 0;            // phares 3/14/98
#else
        sector_t *ss = _g->sectors + i;
        // we must set the address each time, as it is changed by R_FlatNumForName
        spiFlashSetAddress((uint32_t) data + i * sizeof(mapsector_t));
        mapsector_t ms;
        spiFlashGetData(&ms, sizeof(mapsector_t));

        ss->floorheight = SHORT(ms.floorheight) << FRACBITS;
        ss->ceilingheight = SHORT(ms.ceilingheight) << FRACBITS;       

        ss->floorpic = R_FlatNumForName(ms.floorpic);
        ss->ceilingpic = R_FlatNumForName(ms.ceilingpic);
        ss->lightlevel = SHORT(ms.lightlevel);
        ss->special = SHORT(ms.special);
        ss->oldspecial = SHORT(ms.special);
        ss->tag = SHORT(ms.tag);   

        ss->thinglist_sptr = 0;
        ss->touching_thinglist_sptr = 0;            // phares 3/14/98
#endif
    }
}

//
// P_LoadNodes
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadNodes(int lump)
{
    // TODO: remove numnodes and nodes and use p_wad_level
    numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    //    
    nodes = writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);; // cph - wad lump handling updated

    if ((!nodes) || (!numnodes))
    {
        // allow trivial maps
        if (_g->numsubsectors == 1)
            lprintf(LO_INFO, "P_LoadNodes: trivial map (no nodes, one subsector)\n");
        else
            I_Error("P_LoadNodes: no nodes in level");
    }
}

/*
 * P_LoadThings
 *
 * killough 5/3/98: reformatted, cleaned up
 * cph 2001/07/07 - don't write into the lump cache, especially non-idepotent
 * changes like byte order reversals. Take a copy to edit.
 */

static void P_LoadThings(int lump)
{
    int i, numthings = W_LumpLength(lump) / sizeof(mapthing_t);
    const mapthing_t *data = W_CacheLumpNum(lump);
//
    if ((!data) || (!numthings))
        I_Error("P_LoadThings: no things in level");
    printf("Num things %d\r\n", numthings);
    _g->totalstatic = 0;
    spiFlashSetAddress((uint32_t) data);
    for (i = 0; i < numthings; i++)
    {

        // mapthing_t mt = data[i];
        mapthing_t mt;
        spiFlashGetData(&mt, sizeof(mt));
        mt.x = SHORT(mt.x);
        mt.y = SHORT(mt.y);
        mt.angle = SHORT(mt.angle);
        mt.type = SHORT(mt.type);
        mt.options = SHORT(mt.options);

        if (!P_IsDoomnumAllowed(mt.type))
            continue;

        // Do spawn all other stuff.
        P_SpawnMapThing(&mt);
    }
    printf("Num static things %d\r\n", _g->totalstatic);

}

//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//        ^^^
// ??? killough ???
// Does this mean secrets used to be linedef-based, rather than sector-based?
//
// killough 4/4/98: split into two functions, to allow sidedef overloading
//
// killough 5/3/98: reformatted, cleaned up

static void P_LoadLineDefs(int lump, bool isNormalStartup)
{
    int i;
    _g->numlines = W_LumpLength(lump) / sizeof(line_t);
    _g->lines = writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);

    if(isNormalStartup) _g->linedata = Z_Calloc(_g->numlines, sizeof(linedata_t), PU_LEVEL, 0);
    memset(_g->linedata,0,_g->numlines*sizeof(linedata_t));

    for (i = 0; i < _g->numlines; i++)
    {
        _g->linedata[i].special = _g->lines[i].const_special;
    }
}

//
// P_LoadSideDefs
// killough 4/4/98: delay using texture names until
// after linedefs are loaded, to allow overloading.
// killough 5/3/98: reformatted, cleaned up

static void P_LoadSideDefs(int lump, bool isNormalStartup)
{
    // 2021-02-13 next-hack
    // It is stupid to copy everything to ram. therefore we use everything which is in ROM
    // except texture offset, which needs to be copied to ram (it might change).
    // In this way, we save 12 bytes for each side def.
    // now we use only ROM. It is up to the wad to have correct data.
    _g->numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    if(isNormalStartup) _g->textureoffsets = Z_Calloc(_g->numsides, sizeof(*_g->textureoffsets), PU_LEVEL, 0);
    memset(_g->textureoffsets,0,_g->numsides*sizeof(*_g->textureoffsets));
    _g->sides = (side_t*) writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);;
    int i;
    int n = 0, ma = 0, mi = 0;
    for (i = 0; i < _g->numsides; i++)
    {
        register side_t *sd = _g->sides + i;
        _g->textureoffsets[i] = sd->textureoffset;
        // nh: to count how much we can squeeze
        if (n < sd->midtexture)
            n = sd->midtexture;
        if (n < sd->toptexture)
            n = sd->toptexture;
        if (n < sd->bottomtexture)
            n = sd->bottomtexture;
        if (ma < sd->textureoffset)
            ma = sd->textureoffset;
        if (mi > sd->textureoffset)
            mi = sd->textureoffset;
    }
    // 2021-03-13: forgot that switches have changeable textures...
    // is a line special? Then we must reserve a memory location to store its texture numbers
    // first count how many specials we have
    int l = 0;
    for (i = 0; i < _g->numlines; i++)
    {
        if (_g->lines[i].const_special)
        {
            l++;
        }
    }
    // then allocate spaces
    if(isNormalStartup) _g->linesChangeableTextureIndex = Z_Calloc(_g->numlines, sizeof(*_g->linesChangeableTextureIndex), PU_LEVEL, 0);
    if(isNormalStartup) _g->switch_texture_top = Z_Calloc(l, sizeof(*_g->switch_texture_top), PU_LEVEL, 0);
    if(isNormalStartup) _g->switch_texture_mid = Z_Calloc(l, sizeof(*_g->switch_texture_mid), PU_LEVEL, 0);
    if(isNormalStartup) _g->switch_texture_bot = Z_Calloc(l, sizeof(*_g->switch_texture_bot), PU_LEVEL, 0);
    l = 0;
    for (i = 0; i < _g->numlines; i++)
    {
        if (_g->lines[i].const_special)
        {
            // set the changeableTextureIndex
            _g->linesChangeableTextureIndex[i] = l;
            int side0num = _g->lines[i].sidenum[0];
            side_t *sd = _g->sides + side0num;
            // set texture index
            _g->switch_texture_top[l] = sd->toptexture;
            _g->switch_texture_mid[l] = sd->midtexture;
            _g->switch_texture_bot[l] = sd->bottomtexture;
            // point to next slot
            l++;
        }
    }
    printf("Number of textures %d, max offset %d, min offset %d. Special %d\r\n", n, ma, mi, l);
}

static void P_LoadSideDefsTextures()
{
    // 2021-03-23 next-hack
    // This function is called after we stored all the important level data. Here we try to cache (in flash) as many textures as possible
    // _g->sides, _g->numsides shall be already initialized by P_LoadSideDefs.
    int i;
    uint32_t size = 0;
    int32_t *texSizes = Z_Malloc(sizeof(*texSizes) * _g->numtextures, PU_STATIC, NULL);
    memset(texSizes, 0, sizeof(*texSizes) * _g->numtextures);
    for (i = 0; i < _g->numsides; i++)
    {
        // TODO: instead of loading finding everytime TEXTUREx lumps by name we could cache it
        // it would save some time.
        register side_t *sd = _g->sides + i;
        if (!texSizes[sd->midtexture])
        {
            texSizes[sd->midtexture] = getTextureStructSize(sd->midtexture);
            size += texSizes[sd->midtexture];
        }
        if (!texSizes[sd->bottomtexture])
        {
            texSizes[sd->bottomtexture] = getTextureStructSize(sd->bottomtexture);
            size += texSizes[sd->bottomtexture];
        }
        if (!texSizes[sd->toptexture])
        {
            texSizes[sd->toptexture] = getTextureStructSize(sd->toptexture);
            size += texSizes[sd->toptexture];
        }
    }
    Z_Free(texSizes);
    //printf(">>>Size before %d\r\n", size);
    for (i = 0; i < _g->numsides; i++)
    {
        register side_t *sd = _g->sides + i;
        R_GetTexture(sd->midtexture, true, &size);
        R_GetTexture(sd->toptexture, true, &size);
        R_GetTexture(sd->bottomtexture, true, &size);
    }
    //printf(">>>Size after %d\r\n", size);
}

//
// jff 10/6/98
// New code added to speed up calculation of internal blockmap
// Algorithm is order of nlines*(ncols+nrows) not nlines*ncols*nrows
//

#define blkshift 7               /* places to shift rel position for cell num */
#define blkmask ((1<<blkshift)-1)/* mask for rel position within cell */
#define blkmargin 0              /* size guardband around map used */
// jff 10/8/98 use guardband>0
// jff 10/12/98 0 ok with + 1 in rows,cols

typedef struct linelist_t        // type used to list lines in each block
{
    long num;
    struct linelist_t *next;
} linelist_t;

//
// P_LoadBlockMap
//
// killough 3/1/98: substantially modified to work
// towards removing blockmap limit (a wad limitation)
//
// killough 3/30/98: Rewritten to remove blockmap limit,
// though current algorithm is brute-force and unoptimal.
//

static void P_LoadBlockMap(int lump, bool isNormalStartup)
{
    _g->blockmaplump = writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);

    if(isNormalStartup){

        _g->bmaporgx = _g->blockmaplump[0] << FRACBITS;
        _g->bmaporgy = _g->blockmaplump[1] << FRACBITS;
        _g->bmapwidth = _g->blockmaplump[2];
        _g->bmapheight = _g->blockmaplump[3];

    
        // clear out mobj chains - CPhipps - use calloc
        if(isNormalStartup) _g->blocklinks_sptrs = Z_Calloc(_g->bmapwidth * _g->bmapheight, sizeof(*_g->blocklinks_sptrs), PU_LEVEL, 0);
        memset(_g->blocklinks_sptrs,0,_g->bmapwidth * _g->bmapheight*sizeof(*_g->blocklinks_sptrs));

        _g->blockmap = _g->blockmaplump + 4;
    }
}

//
// P_LoadReject - load the reject table, padding it if it is too short
// totallines must be the number returned by P_GroupLines()
// an underflow will be padded with zeroes, or a doom.exe z_zone header
// 
// this function incorporates e6y's RejectOverrunAddInt code:
// e6y: REJECT overrun emulation code
// It's emulated successfully if the size of overflow no more than 16 bytes.
// No more desync on teeth-32.wad\teeth-32.lmp.
// http://www.doomworld.com/vb/showthread.php?s=&threadid=35214

static void P_LoadReject(int lumpnum)
{
    int lump = lumpnum + ML_REJECT;
    _g->rejectmatrix = writeLumpToFlashRegion(lump, FLASH_LEVEL_REGION, true);
}

//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
// killough 5/3/98: reformatted, cleaned up
// cph 18/8/99: rewritten to avoid O(numlines * numsectors) section
// It makes things more complicated, but saves seconds on big levels
// figgi 09/18/00 -- adapted for gl-nodes

// cph - convenient sub-function
static void P_AddLineToSector(const line_t *li, sector_t *sector)
{
    //sector->lines[sector->linecount++] = li;
    getSectorLines(sector)[sector->linecount++] = li; 
}
// modified to return totallines (needed by P_LoadReject)
static int P_GroupLines(void)
{
    register const line_t *li;
    register sector_t *sector;
    int i, j, total = _g->numlines;

    // figgi
    for (i = 0; i < _g->numsubsectors; i++)
    {
        const seg_t *seg = &_g->segs[_g->subsectors[i].firstline];
        _g->subsectors[i].sector = NULL;
        for (j = 0; j < _g->subsectors[i].numlines; j++)
        {
            if (seg->sidenum != NO_INDEX)
            {
                _g->subsectors[i].sector = &_g->sectors[_g->sides[seg->sidenum].sector_num];
                break;
            }
            seg++;
        }
        if (_g->subsectors[i].sector == NULL)
            I_Error("P_GroupLines: Subsector a part of no sector!\n");
    }

    for(int i=0;i<_g->numsectors;i++){
        _g->sectors[i].linecount=0; //alecamar: fix. TIt aasumed this was reset in LoadSectors
    }

    // count number of lines in each sector
    for (i = 0, li = _g->lines; i < _g->numlines; i++, li++)
    {
        LN_FRONTSECTOR(li)->linecount++;
        if (LN_BACKSECTOR(li) && LN_BACKSECTOR(li) != LN_FRONTSECTOR(li))
        {
            LN_BACKSECTOR(li)->linecount++;
            total++;
        }
    }
// now we have the total number of lines.
    line_t **linebuffer = Z_Malloc(total * sizeof(line_t*), PU_STATIC, 0);
    line_t **lineBufferArray = linebuffer;
    {  // allocate line tables for each sector



        for (i = 0, sector = _g->sectors; i < _g->numsectors; i++, sector++)
        {
            sector->lines_ppptr = linebuffer;
            linebuffer += sector->linecount;
            //linebuffer_ppptr += sector->linecount;
            sector->linecount = 0;
        }
    }

    // Enter those lines
    for (i = 0, li = _g->lines; i < _g->numlines; i++, li++)
    {
        P_AddLineToSector(li, LN_FRONTSECTOR(li));
        if (LN_BACKSECTOR(li) && LN_BACKSECTOR(li) != LN_FRONTSECTOR(li))
            P_AddLineToSector(li, LN_BACKSECTOR(li));
    }

    // store buffer to flash
    line_t **flashArrayAddress = writeBufferToFlashRegion(lineBufferArray, total * sizeof(line_t*), FLASH_LEVEL_REGION, true);
    // now we need to update again all the pointers
    for (i = 0, sector = _g->sectors; i < _g->numsectors; i++, sector++)
    {
        sector->lines_ppptr = flashArrayAddress;
        flashArrayAddress += sector->linecount;
    }
    // now we can free the buffer
    Z_Free(lineBufferArray);
    // calculate the box that contains each sector.
    // sound will be generated from the center of this box.
    for (i = 0, sector = _g->sectors; i < _g->numsectors; i++, sector++)
    {
        fixed_t bbox[4];
        M_ClearBox(bbox);

        for (int l = 0; l < sector->linecount; l++)
        {
            const line_t *pline = getSectorLineByIndex(sector, l);

            M_AddToBox(bbox, pline->v1.x, pline->v1.y);
            M_AddToBox(bbox, pline->v2.x, pline->v2.y);
        }
        sector->soundorg.x = bbox[BOXRIGHT] / 2 + bbox[BOXLEFT] / 2;
        sector->soundorg.y = bbox[BOXTOP] / 2 + bbox[BOXBOTTOM] / 2;
    }

    return total; // this value is needed by the reject overrun emulation code
}

void P_FreeLevelData()
{
    R_ResetPlanes();

    Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

    Z_Free(_g->braintargets);
    _g->braintargets = NULL;
    _g->numbraintargets_alloc = _g->numbraintargets = 0;
}


int lastEpisode = 0;
int lastMap = 0;
int lastPlayerMask = 0;
skill_t lastSkill;

void P_SetupLevelFromCheckpoint(){
    P_SetupLevel(true, lastEpisode, lastMap,lastPlayerMask,lastSkill);
}

#include "z_bmalloc.h"
DECLARE_BLOCK_MEMORY_ALLOC_ZONE(secnodezone);
DECLARE_BLOCK_MEMORY_ALLOC_ZONE(mobjzone);
DECLARE_BLOCK_MEMORY_ALLOC_ZONE(static_mobjzone);
extern uint32_t currentLevelFlashOffset;

void P_SetupLevel(bool fromCheckpoint, int episode, int map, int playermask, skill_t skill)
{
    bool isNormalStartup = !fromCheckpoint;

    //
    if(isNormalStartup){ //If normal, store the arguments so that they can be used by the checkpoint restore later to recache everything
        lastEpisode = episode;
        lastMap = map;
        lastPlayerMask = playermask;
        lastSkill = skill;
    }

    if(isNormalStartup) printf("Level setup. From checkpoint: %d\r\n",fromCheckpoint);


    
    //
    cachedColumnOffsetDataPatch = NULL;
    // every boot, we need to refresh level data, to be sure that corrupted data
    // due to  resets or power down wont screw up everything.

    char lumpname[9];
    int lumpnum;


    if(isNormalStartup){

        NULL_BLOCK_MEMORY_ALLOC_ZONE(secnodezone);
        NULL_BLOCK_MEMORY_ALLOC_ZONE(mobjzone);
        NULL_BLOCK_MEMORY_ALLOC_ZONE(static_mobjzone);

        _g->totallive = _g->totalkills = _g->totalitems = _g->totalsecret = _g->wminfo.maxfrags = 0;
        _g->wminfo.partime = 180;

        _g->player.killcount = 0;
        _g->player.secretcount = 0;
        _g->player.itemcount = 0;

        // Initial height of PointOfView will be set by player think.
        _g->player.viewz = 1;
    }
  

     if(isNormalStartup) k_screen_RenderLoadingScreen(50,"Loading map data...");

    if(isNormalStartup){
        // Make sure all sounds are stopped before Z_FreeTags.
        S_Start();
        P_FreeLevelData();
    }    

    currentLevelFlashOffset = sizeof(wad_level_flash_data_t); //If we set this to 0 it crashes. Basically we are saying that there isn't anything interesting stored in flash (set next address to this)

    // now that we have freed level data, we can allocate new temporary buffers.
    initLumpPtrTable();

    memset(textures, 0, _g->numtextures * sizeof *textures);    


    if(isNormalStartup){
        _g->skyflatnum = R_FlatNumForName( SKYFLATNAME);
        P_InitThinkers();
    }   

    // find map name
    if (_g->gamemode == commercial)
    {
        sprintf(lumpname, "MAP%02d", map);         // killough 1/24/98: simplify
    }
    else
    {
        sprintf(lumpname, "E%dM%d", episode, map); // killough 1/24/98: simplify
    }

    lumpnum = W_GetNumForName(lumpname);

    if(isNormalStartup){
        _g->leveltime = 0;
        _g->totallive = 0;
    }

    //HERE IT WORKS

    //FLASH WRITE SIMPLE
    printf("P_LoadVertexes\r\n");
    P_LoadVertexes(lumpnum + ML_VERTEXES);



    //RAM WRITE    
    if(isNormalStartup){
        printf("P_LoadSectors\r\n");
        P_LoadSectors(lumpnum + ML_SECTORS,isNormalStartup);  
    }


    //FLASH WRITE SIMPLE
    printf("P_LoadLineDefs\r\n");
    P_LoadLineDefs(lumpnum + ML_LINEDEFS,isNormalStartup);


    
    //FLASH WRITE SIMPLE
    printf("P_LoadSideDefs\r\n");
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS,isNormalStartup);


   
    //FLASH WRITE SIMPLE
    printf("P_LoadBlockMap\r\n");
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP,isNormalStartup);
  /*      if(!isNormalStartup){
        return;
    }  */

    //RAM WRITE
    printf("P_LoadSubsectors\r\n");
    P_LoadSubsectors(lumpnum + ML_SSECTORS,isNormalStartup);     

    //FLASH WRITE SIMPLE
    printf("P_LoadNodes\r\n");
    P_LoadNodes(lumpnum + ML_NODES); // 0k

    //FLASH WRITE SIMPLE
    printf("P_LoadSegs\r\n");
    P_LoadSegs(lumpnum + ML_SEGS);  // 0k

    //FLASH WRITE SIMPLE + PROCESSING
    printf("P_GroupLines\r\n");
    P_GroupLines();
    





    //FLASH WRITE SIMPLE
    printf("P_LoadReject\r\n");
    P_LoadReject(lumpnum);  // 0k


    if(isNormalStartup){
    /* cph - reset all multiplayer starts */
        memset(_g->playerstarts, 0, sizeof(_g->playerstarts));

        for (int i = 0; i < MAXPLAYERS; i++)
            _g->player.mo = NULL;

        P_MapStart();
    }

    if(isNormalStartup) k_screen_RenderLoadingScreen(60,"Loading NPCs...");

    if(isNormalStartup){
        P_LoadThings(lumpnum + ML_THINGS);

        if (_g->playeringame && !_g->player.mo)
        {
            I_Error("P_SetupLevel: missing player start\n");
        }

        // killough 3/26/98: Spawn icon landings:
        if (_g->gamemode == commercial)
            P_SpawnBrainTargets();

        // set up world state
        P_SpawnSpecials(); // 1k

        P_MapEnd();
    }  
    

    if(isNormalStartup) k_screen_RenderLoadingScreen(70,"Caching texture data...");


    //FLASH WRITE COMPLEX
    lprintf(LO_INFO, "P_InitPicAnims");
    P_InitPicAnims();


    //FLASH WRITE COMPLEX
    if (_g->gamemode == commercial)
    {
        _g->skytexture = R_LoadTextureByName("SKY3", true);
        if (_g->gamemap < 12) _g->skytexture = R_LoadTextureByName("SKY1", true);
        else if (_g->gamemap < 21) _g->skytexture = R_LoadTextureByName("SKY2", true);
    }
    else
    {
        switch (_g->gameepisode)
        {
            case 1:
                _g->skytexture = R_LoadTextureByName("SKY1", true);
                break;
            case 2:
                _g->skytexture = R_LoadTextureByName("SKY2", true);
                break;
            case 3:
                _g->skytexture = R_LoadTextureByName("SKY3", true);
                break;
            case 4: // Special Edition sky
                _g->skytexture = R_LoadTextureByName("SKY4", true);
                break;
        } //jff 3/27/98 end sky setting fix
    }      
       
    //FLASH WRITE COMPLEX
    for (int j = 0; j < _g->numsectors; j++)
    {
        int picnum;
        picnum = _g->sectors[j].ceilingpic + _g->firstflat;
        getAddressOrCacheLumpNum(picnum, true, FLASH_LEVEL_REGION);
        picnum = _g->sectors[j].floorpic + _g->firstflat;
        getAddressOrCacheLumpNum(picnum, true, FLASH_LEVEL_REGION);
    }


    //FLASH WRITE COMPLEX
    P_LoadSideDefsTextures();
    

    if(isNormalStartup) k_screen_RenderLoadingScreen(90,"Saving cached data...");

    //FLASH WRITE COMPLEX
    // now cache every pointer so that in game we do not need to save to flash.
    for (int i = 0; i < _g->numlumps; i++)
    {
        getAddressOrCacheLumpNum(i, false, FLASH_LEVEL_REGION);
    }

    //FLASH WRITE SIMPLE
    storeLumpArrayToFlash(true);
}

//
// P_Init
//
void P_Init(void)
{
    lprintf(LO_INFO, "P_InitSwitchList");
    P_InitSwitchList();
// moved to setup level
//    lprintf(LO_INFO, "P_InitPicAnims");
//    P_InitPicAnims();

    lprintf(LO_INFO, "R_InitSprites");
    R_InitSprites(sprnames);
}
