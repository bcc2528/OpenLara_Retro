#include "game.h"

EWRAM_DATA int32 fps;
EWRAM_DATA int32 frameIndex = 0;
EWRAM_DATA int32 fpsCounter = 0;
EWRAM_DATA uint32 curSoundBuffer = 0;

const void* TRACKS_AD4;
const void* TITLE_SCR;
const void* levelData;

char	egb_work[1536];
char	snd_work[16384];

//LARGE_INTEGER g_timer;
//LARGE_INTEGER g_current;

void Timer_Counter(void)
{
	frameIndex++;
	fpsCounter++;
}

uint8 rgb_table[32] =
{0, 8, 16, 25, 33, 41, 49, 58, 66, 74, 82, 90, 99, 107, 115, 123, 132, 140, 148, 156, 165, 173, 181, 189, 197, 206, 214, 222, 230, 239, 247, 255};

void osSetPalette(const uint16* palette)
{
	uint16 c;

	for(int i = 0;i < 256;i++)
	{
		c = palette[i];
		outp(0xfd90,i);
		outp(0xfd94,rgb_table[c & 31]);
		outp(0xfd96,rgb_table[(c >> 5) & 31]);
		outp(0xfd92,rgb_table[(c >> 10) & 31]);
	}
}

int32 osGetSystemTimeMS()
{
    return frameIndex;
}

bool osSaveSettings()
{
    FILE* f = fopen("settings.dat", "wb");
    if (!f) return false;
    fwrite(&gSettings, sizeof(gSettings), 1, f);
    fclose(f);
    return true;
}

bool osLoadSettings()
{
    FILE* f = fopen("settings.dat", "rb");
    if (!f) return false;
    uint8 version;
    fread(&version, 1, 1, f);
    if (version != gSettings.version) {
        fclose(f);
        return false;
    }
    fread((uint8*)&gSettings + 1, sizeof(gSettings) - 1, 1, f);
    fclose(f);
    return true;
}

bool osCheckSave()
{
    FILE* f = fopen("savegame.dat", "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool osSaveGame()
{
    FILE* f = fopen("savegame.dat", "wb");
    if (!f) return false;
    fwrite(&gSaveGame, sizeof(gSaveGame), 1, f);
    fwrite(&gSaveData, gSaveGame.dataSize, 1, f);
    fclose(f);
    return true;
}

bool osLoadGame()
{
    FILE* f = fopen("savegame.dat", "rb");
    if (!f) return false;

    uint32 version;
    fread(&version, sizeof(version), 1, f);

    if (SAVEGAME_VER != version)
    {
        fclose(f);
        return false;
    }

    fread(&gSaveGame.dataSize, sizeof(gSaveGame) - sizeof(version), 1, f);
    fread(&gSaveData, gSaveGame.dataSize, 1, f);
    fclose(f);
    return true;
}

void osJoyVibrate(int32 index, int32 L, int32 R) {}

uint8 *soundBuffer;

int32 snd_pos;
int32 snd_count;
int32 snd_sync;

void soundFill()
{
	int32 limit;
	limit = 9;

	if (snd_sync >= 34)
	{
		limit = 8;
	}

	if(snd_count >= limit)
	{
		sndFill(soundBuffer + ((snd_pos) + 32));
		snd_pos += SND_SAMPLES;
		if(snd_pos >= (SND_SAMPLES * 8))
		{
			snd_pos = 0;
		}
		if(limit == 8)
		{
			snd_sync = 0;
		}
		else
		{
			snd_sync++;
		}
		snd_count = 0;
	}
	else
	{
		snd_count++;
	}
}

void soundInit()
{
	sndInit();

	SND_init(snd_work);
	SND_fm_timer_b_set(0, 0);
	SND_int_timer_b_set(Timer_Counter);
	SND_fm_timer_b_set(255, 140); // (256 - 140 = 116) * 12 * 192 = 33.408ms
	SND_fm_timer_b_start();

	soundBuffer = (uint8 *)malloc(32 + (SND_SAMPLES * 8));
	memset(soundBuffer + 32, 0x80, (SND_SAMPLES * 8));
	// PCM Data Header
	*(uint32 *)&soundBuffer[0] = 0; // Sound name
	*(uint32 *)&soundBuffer[4] = 0; // Sound name
	*(uint32 *)&soundBuffer[8] = 0x80386; // Sound ID
	*(uint32 *)&soundBuffer[12] = (SND_SAMPLES * 8); // Data length
	*(uint32 *)&soundBuffer[16] = 0; // Loop start address
	*(uint32 *)&soundBuffer[20] = (SND_SAMPLES * 8); // Loop length
	*(uint16 *)&soundBuffer[24] = 0x409; // Frequency 10507.3Hz
	*(uint16 *)&soundBuffer[26] = 0; // Frequency correction
	soundBuffer[28] = 0x3c; // Scale
	soundBuffer[29] = 0x0; // Reserve1
	*(uint16 *)&soundBuffer[30] = 0; // Reserve2

	snd_pos = (SND_SAMPLES * 1);
	snd_count = 9;
	snd_sync = 0;

	SND_elevol_mute(0x01);
	SND_pcm_mode_set(1);
	SND_pan_set(71, 64);
	SND_pcm_play(71, 0x3c, 127, (char *)soundBuffer);

	SND_fm_timer_a_set(0, 0);
	SND_int_timer_a_set(soundFill);
	SND_fm_timer_a_set(255, 483); // (1024 - 483 = 541) * 12 * 12 / 8000(KHz) = 9.738 ms
	SND_fm_timer_a_start();
}

void blit()
{
	char    para[16];

	DWORD( para+0  ) = (intptr_t)&fb[0];
	WORD( para+4  ) = 0x14;
	WORD( para+6  ) = 39;
	WORD( para+8  ) = 39;
	WORD( para+10 ) = FRAME_WIDTH + 38;
	WORD( para+12 ) = FRAME_HEIGHT + 38;

	EGB_putBlock( egb_work, 0, (void *)para );

	/*uint8 __far *vram = (uint8 __far *)MK_FP(0x010C, 0x00009C27);
	uint8 *p = (uint8 *)&fb[0];

	int i = 0;
	do
	{
		_fmemcpy(vram, p, FRAME_WIDTH);
		vram+=1024;
		p+=FRAME_WIDTH;
	} while(++i < FRAME_HEIGHT);*/
}

void openlara_exit(void)
{
	EGB_init(egb_work, 1536);

	KYB_clrbuf();
	KYB_init();
	KYB_clic ( 1 );

	SND_end();

	exit(0);
}

#define KB_BREAK  0x7C
#define KB_1       0x2
#define KB_2       0x3
#define KB_3       0x4
#define KB_4       0x5
#define KB_A      0x1E
#define KB_S      0x1F
#define KB_D      0x20
#define KB_Z      0x2A
#define KB_X      0x2B
#define KB_C      0x2C
#define KB_UP     0x4D
#define KB_LEFT   0x4F
#define KB_RIGHT  0x51
#define KB_DOWN   0x50
#define KB_ENTER  0x1D
#define KB_SPACE  0x35
#define KB_SHIFT  0x53

uint8 keycheck[128];

void inputUpdate()
{
	uint32    keyscan, encode; 

	keys = 0;

	while(1)
	{
		keyscan = KYB_read( 1, &encode );

		if ( (keyscan >> 8 ) == 0xff )
		{
			break;
		}
		else if( (keyscan >> 7 ) == 0)
		{
			keycheck[(keyscan & 127)] = 1;
		}
		else
		{
			keycheck[(keyscan & 127)] = 0;
		}
	}

	if (keycheck[KB_UP] == 1)		keys |= IK_UP;
	if (keycheck[KB_RIGHT] == 1)		keys |= IK_RIGHT;
	if (keycheck[KB_DOWN] == 1)		keys |= IK_DOWN;
	if (keycheck[KB_LEFT] == 1)		keys |= IK_LEFT;

	if (keycheck[KB_SHIFT] == 1)		keys |= IK_X;
	if (keycheck[KB_S] == 1)		keys |= IK_Y;
	if (keycheck[KB_D] == 1)		keys |= IK_Z;
	if (keycheck[KB_Z] == 1)		keys |= IK_A;
	if (keycheck[KB_X] == 1)		keys |= IK_B;
	if (keycheck[KB_C] == 1)		keys |= IK_C;

	if (keycheck[KB_ENTER] == 1)		keys |= IK_SELECT;

	if (keycheck[KB_1] == 1) players[0]->extraL->goalWeapon = WEAPON_PISTOLS;
	if (keycheck[KB_2] == 1) players[0]->extraL->goalWeapon = WEAPON_MAGNUMS;
	if (keycheck[KB_3] == 1) players[0]->extraL->goalWeapon = WEAPON_UZIS;
	if (keycheck[KB_4] == 1) players[0]->extraL->goalWeapon = WEAPON_SHOTGUN;

	if (keycheck[KB_BREAK] == 1)		openlara_exit();
}

const void* osLoadScreen(LevelID id)
{
    return TITLE_SCR;
}

const void* osLoadLevel(LevelID id)
{
    // level1
    char buf[32];

    delete[] levelData;

    sprintf(buf, "data/%s.PKD", (const char*)gLevelInfo[id].data);

    FILE *f = fopen(buf, "rb");

    if (!f)
        return NULL;

    {
        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8* data = new uint8[size];
        fread(data, 1, size, f);
        fclose(f);

        levelData = data;
    }

// tracks
    if (!TRACKS_AD4)
    {
        FILE *f = fopen("data/TRACKS.AD4", "rb");
        if (!f)
            return NULL;

        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8* data = new uint8[size];
        fread(data, 1, size, f);
        fclose(f);

        TRACKS_AD4 = data;
    }

    if (!TITLE_SCR)
    {
        FILE *f = fopen("data/TITLE.SCR", "rb");
        if (!f)
            return NULL;

        fseek(f, 0, SEEK_END);
        int32 size = ftell(f);
        fseek(f, 0, SEEK_SET);
        uint8* data = new uint8[size];
        fread(data, 1, size, f);
        fclose(f);

        TITLE_SCR = data;
    }
    
    return (void*)levelData;
}

int main(void)
{
	EGB_init(egb_work, 1536);
	EGB_resolution( egb_work, 0, 12 );

	EGB_displayStart( egb_work, 2, 2, 2 );
	EGB_displayStart( egb_work, 0, 0, 0 );

	EGB_displayPage( egb_work, 0, 3 );
	EGB_writePage( egb_work, 0 );

	EGB_color( egb_work, 0, 0x00 );
	EGB_clearScreen( egb_work );

	KYB_init();
	KYB_setcode( 0x4000 );
	KYB_clic ( 1 );
	memset(keycheck, 0, sizeof(keycheck));

	soundInit();

	gameInit();

	int32 startTime = frameIndex;
	frameIndex++;
	int32 lastFrameIndex = -1;
	int32 fpsIndex = 0;

	do {
		inputUpdate();

		int32 count = frameIndex - lastFrameIndex;
		gameUpdate(count);
		lastFrameIndex = frameIndex;

		gameRender();

		blit();

		fpsIndex++;
		if(fpsCounter >= 30)
		{
			if(fpsIndex >= 30) fpsIndex = 30;
			fps = fpsIndex;
			fpsIndex = 0;
			fpsCounter = 0;
		}
	} while (1);

	return 0;
}
