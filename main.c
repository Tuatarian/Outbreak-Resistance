#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#define SFOR(name, l, u) for (int name = l; name < u; ++name)

#define RAND_UNIT() (float)GetRandomValue(0, RAND_MAX)/(float)RAND_MAX

#define BGREY (Color) { 0x02, 0x02, 0x02, 255 }
#define AGREY (Color){ 0x66, 0x66, 0x66, 255 }
#define FWHITE (Color){ 235, 235, 235, 0 }

const int SCREEN_W = 1920;
const int SCREEN_H = 1000;

int nCrit = 7;
int nInfSpawn = 10;

typedef struct Cell {
    float res;
    bool infected;
    bool crit;
    bool land;
    int locx, locy;
} Cell;

void drawGrid(int nx) {
    int w = GetScreenWidth()/nx;
    DrawLine(GetScreenWidth(), 0, GetScreenWidth(), GetScreenHeight(), FWHITE);
    for (int i = 0; i < nx; ++i) {
	DrawLine(w*i, 0, w*i, GetScreenHeight(), FWHITE);
    }

    int ny = GetScreenHeight()/w;
    DrawLine(0, GetScreenHeight(), GetScreenWidth(), GetScreenHeight(), FWHITE);
    for (int i = 0; i < ny; ++i) {
	DrawLine(0, w*i, GetScreenWidth(), w*i, FWHITE);
    }
}

Cell* getCellPtr(Cell** map, Cell c) {
    int x = c.locx; int y = c.locy; return &map[x][y];
}

Cell* getNbor(Cell c, int dir, Cell** map, int mapx, int mapy) { // 0 is up, clockwise
    int x = c.locx; int y = c.locy;
    switch(dir) {
    case 0:
	if (y - 1 < 0) return NULL;
	return &map[x][y - 1];
	break;
    case 1:
	if (x + 1 >= mapx) return NULL;
	return &map[x + 1][y];
	break;
    case 2:
	if (y + 1 >= mapy) return NULL;
	return &map[x][y + 1];
	break;
    case 3:
	if (x - 1 < 0) return NULL;
	return &map[x -1][y];
	break;
    default:
	printf("invalid arg to getNbor\n");
	exit(1);
    }
}

void makeContinent(Cell** map, int mapx, int mapy, Cell** land, int* landlen, int iters, float threshold) { // Writes to land
    for (int throw = 0; throw < iters; ++throw) {
	int curI = GetRandomValue(0, *landlen - 1);
	Cell cur = *land[curI];
	for (int j = 0; j < 4; ++j) {
	    Cell* nbor = getNbor(cur, j, map, mapx, mapy);
	    if (!nbor || nbor->land) continue;
		
	    float randf = (float)GetRandomValue(0, RAND_MAX)/(float)RAND_MAX;
	    if (randf > threshold) {
		land[(*landlen)++] = nbor;
		nbor->land = true;
	    }
	}
    }
}

void makeContinent2(Cell** map, int mapx, int mapy, Cell** land, int* landlen, int iters, float threshold) {
    for (int throw = 0; throw < iters; ++throw) {
	for (int i = 0; i < mapx; ++i) {
	    for (int j = 0; j < mapy; ++j) {
		if (map[i][j].land) continue;
		
		float randf = -1.0;
		for (int k = 0; k < 4; ++k) {
		    Cell* nbor = getNbor(map[i][j], k, map, mapx, mapy);
		    if (!nbor) continue;
		    if (nbor->land) {
			randf = (float)GetRandomValue(0, RAND_MAX)/(float)RAND_MAX;
			break;
		    }
		}
		if (randf > threshold) {
		    land[(*landlen)++] = &map[i][j];
		    map[i][j].land = true;
		}
	    }
	}
    }
}

void colorCell(int x, int y, int garrx, int garry, Color col) {
    int w = GetScreenWidth()/garrx;
    
    int l = w*x + 1;
    int u = w*y + 1;
    int r = w*(x + 1) - 1;
    int b = w*(y + 1) - 1;

    if (x == garrx - 1) r = GetScreenWidth() - 1;
    if (y == garry - 1) b = GetScreenHeight() - 1;

    DrawRectangle(l, u, r - l, b - u, col);
}

void cleanInfected(Cell** infectados, int* infelen) {
    SFOR(k, 0, *infelen) {
	if (!infectados[k]->infected) infectados[k] = infectados[--(*infelen)];
    }
}

void resetMap(Cell** map, int mapx, int mapy) {
    for (int i = 0; i < mapx; ++i) {
	for (int j = 0; j < mapy; ++j) {
	    map[i][j] = (Cell){0.0, false, false, false, i, j};
	}
    }
}

void reset( Cell** map, int mapx, int mapy, int* landlen, Cell** land, int* infelen, Cell** infectados, float* drugAmt, float* totalDrugs, int numInfSpawn, int numCrits, int* dcenters) {
    resetMap(map, mapx, mapy);
    
    *landlen = 1;
    *infelen = 0;
    *drugAmt = 0.0;
    *totalDrugs = 0.0;
    *dcenters = numCrits;

    makeContinent(map, mapx, mapy, land, landlen, 10000, 0.10);

    SFOR(throw, 0, numCrits) {
	int tile = GetRandomValue(0, *landlen - 1);
	if (land[tile]->crit) {
	    throw += -1;
	    continue;
	}
	land[tile]->crit = true;
    }

    // Infection Seeds
    SFOR(throw, 0, numInfSpawn) {
	int tile = GetRandomValue(0, *landlen - 1);
	if (land[tile]->crit || land[tile]->infected) {
	    throw += -1;
	    continue;
	}
	land[tile]->infected = true;
	land[tile]->res = RAND_UNIT();
	infectados[(*infelen)++] = land[tile];
    }
}

void DrawTextCentered(const char* str, int x, int y, int fsize, Color col) {
    int fz = (fsize < 20) ? 20 : fsize;
    Vector2 measure = MeasureTextEx( GetFontDefault(), str, (float)fsize, fz/20.0);
    DrawText( str, x - (int)(measure.x/2), y - (int)(measure.y/2), fsize, col );
}

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "gameusz");
    SetTargetFPS(60);

    InitAudioDevice();
    SetMasterVolume(1.0);
    
    Sound drugAdmin = LoadSound("assets/sfx/drugAdmin.wav");
    Sound placeCenter = LoadSound("assets/sfx/centerPlace.wav");
    Sound clap = LoadSound("assets/sfx/clap.wav");

    Music gamelan = LoadMusicStream("assets/gamelan.mp3");
    SetMusicVolume(gamelan, 0.5);
    PlayMusicStream(gamelan);

    bool paused = true;
    bool won = false;
    bool tutorial = true;

    {
	FILE* file = fopen("lvl.txt", "r");
	nCrit = fgetc(file) - '0';
	fclose(file);
    }
    
    // Creating the grid
    int garrx = 80;
    int grid_w = GetScreenWidth()/garrx;
    int garry = GetScreenHeight()/grid_w;
    Cell** map = malloc(garrx * sizeof(Cell*));
    for (int i = 0; i < garrx; ++i) {
	map[i] = malloc(garry * sizeof(Cell));
	for (int j = 0; j < garry; ++j) {
	    map[i][j] = (Cell){0.0, false, false, false, i, j};
	}
    }

    Cell** land = malloc( garrx * garry * sizeof(Cell*));
    int landlen = 0;
    land[landlen++] = &map[garrx/2][garry/2];
    map[garrx/2][garry/2].land = true;
    int nSeedsMap = GetRandomValue(2, 7);
    SFOR(i, 0, nSeedsMap) {
	int x = GetRandomValue(0, garrx); int y = GetRandomValue(0, garry);
	if (map[x][y].land) {
	    i += -1;
	    continue;
	}
	map[x][y].land = true;
	land[landlen++] = &map[x][y];
    }
    //printf("%d\n", nSeedsMap);
    makeContinent(map, garrx, garry, land, &landlen, 10000, 0.10);

    // Generate Infection and Vital organs
    SFOR(throw, 0, nCrit) {
	int tile = GetRandomValue(0, landlen - 1);
	if (land[tile]->crit) {
	    throw += -1;
	    continue;
	}
	land[tile]->crit = true;
    }

    // Infection Seeds
    Cell** infectados = malloc(4 * garrx * garry * sizeof(Cell*));
    int infelen = 0;
    SFOR(throw, 0, nInfSpawn) {
	int tile = GetRandomValue(0, landlen - 1);
	if (land[tile]->crit || land[tile]->infected) {
	    throw += -1;
	    continue;
	}
	land[tile]->infected = true;
	land[tile]->res = RAND_UNIT();
	infectados[infelen++] = land[tile];
    }


    float drugAmt = 0.0;
    float totalDrugs = 0.0;
    int dcenters = nCrit;
    
    // Main Loop
    while (!WindowShouldClose()) {
	BeginDrawing();
	UpdateMusicStream(gamelan);
	DrawRectangle(0, 0, SCREEN_W, SCREEN_H, AGREY);
	int dwlen = snprintf(NULL, 0, "Drug Amount: %.2f", drugAmt);
	int sclen = snprintf(0, 0, "Total Drugs: %.2f", totalDrugs);
	char* dw = malloc(dwlen + 1);
	char* sw = malloc(sclen + 1);
	snprintf(dw, dwlen + 1, "Drug Amount: %.2f", drugAmt);
	snprintf(sw, sclen + 1, "Total Drugs: %.2f", totalDrugs);
	
	drawGrid(garrx);
	for (int i = 0; i < landlen; ++i) {
	    Color col = BGREY;
	    if (land[i]->infected) {
		col = (Color) { (int)(land[i]->res * 255.0), (int)((1.0 - land[i]->res) * 255.0), 0, 255 };
	    } else if (land[i]->crit) {
		col = YELLOW;
	    }
	    colorCell(land[i]->locx, land[i]->locy, garrx, garry, col);
	}

	if (IsKeyPressed(KEY_T)) tutorial = false;

	if (tutorial) {
	    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 127});
	    DrawText("A colored cell represents an infected tile", 40, 80, 40, WHITE);
	    DrawText("The redder the cell, the lower the probability that the drug will cure it", 40, 120, 40, WHITE);

	    DrawText("Left-Click to spend 1 drug to attempt to cure cells in a small area surrounding the cursor", 40, 300, 40, WHITE);
	    DrawText("Press SPACE to pause gameplay at any time", 40, 340, 40, WHITE);

            DrawText("The golden tiles are drug production centers", 40, 480, 40, YELLOW);
            DrawText("Each one will produce some amount of drug each second. Losing them all loses the game", 40, 520, 40, YELLOW);
	    DrawText("Right-Click or press F on an empty tile to spend 10 drugs to build a production center", 40, 560, 40, YELLOW);

	    DrawText("The goal is to survive until 80 total drugs are produced", 40, 640, 40, GREEN);
	    DrawText("There are 7 rounds of increasing difficulty", 40, 680, 40, GREEN);
	    DrawText("modulated by the starting number of production centers", 40, 720, 40, GREEN);

	    DrawText("Press T to exit the tutorial", 40, 800, 40, MAGENTA);
	    DrawText("Good Luck!", 40, 840, 40, MAGENTA);
	    EndDrawing();
	    continue;
	}
	    
	// Update Tick
	if (IsKeyPressed(KEY_SPACE)) paused = !paused;
	if (!paused) {
	    float deltaDrugs = 1.0/60.0 * (float)dcenters;
	    drugAmt += deltaDrugs;
            totalDrugs += deltaDrugs;

	    SFOR(i, 0, garrx) {
		SFOR(j, 0, garry) {
		    Cell* c = &map[i][j];
		    if (c->infected) {
			SFOR(k, 0, 4) {
			    Cell* nbor = getNbor(*c, k, map, garrx, garry);
			    if (!nbor || !nbor->land || nbor->infected) continue;

			    if (RAND_UNIT() > 0.99) {
				nbor->infected = true;
				infectados[infelen++] = nbor;
				if (nbor->crit) {
				    nbor->crit = false; dcenters--;
				}
				float minNres = 1.0, maxNres = 0.0;
				SFOR(k, 0, 4) {
				    Cell* nbor2 = getNbor(*nbor, k, map, garrx, garry);
				    if (!nbor2 || !nbor2->infected) continue;
				    if (nbor2->res < minNres) minNres = nbor2->res;
				    if (nbor2->res > maxNres) maxNres = nbor2->res;
				}
				nbor->res = (maxNres + 3.0*minNres)*0.25*0.9 + 0.1*RAND_UNIT();
			    }
			}
		    }
		}
	    }
	    
	} else {
	    DrawText("Paused", 40, 50, 50, YELLOW);
	}

	// Drug Administration
	if ( IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && drugAmt >= 1.0 ) {
	    Vector2 mpos = GetMousePosition();
	    int x = (mpos.x/grid_w < garrx - 1) ? mpos.x/grid_w : garrx - 1;
	    int y = (mpos.y/grid_w < garry - 1) ? mpos.y/grid_w : garry - 1;

	    drugAmt += -1.0;
	    PlaySound(drugAdmin);

	    SFOR(i, x - 3, x + 3) {
		if (i < 0 || i >= garrx) continue;
		SFOR(j, y - 3, y + 3) {
		    if (j < 0 || j >= garry) continue;
                    //if (abs(x - i) + abs(y - j) >= 4) continue;
		    
		    Cell* c = &map[i][j];
		    
		    float r = RAND_UNIT();
		    //printf("%.2f %.2f \n", r, c->res);
		    if (c->infected && r >= c->res) {
			c->infected = false;
			cleanInfected(infectados, &infelen);
		    }
		}
	    }
	}

	if ( (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || IsKeyPressed(KEY_F)) && drugAmt >= 10.0) {
	    Vector2 mpos = GetMousePosition();
	    int x = (mpos.x/grid_w < garrx - 1) ? mpos.x/grid_w : garrx - 1;
            int y = (mpos.y/grid_w < garrx - 1) ? mpos.y/grid_w : garry - 1;

	    Cell* c = &map[x][y];
	    if (!c->infected && c->land) {
		drugAmt += -10.0;
		PlaySound(placeCenter);
		dcenters++;
		c->crit = true;
		c->res = 0.0;
	    }
	}


	//printf("%d %d %d\n", dcenters, infelen - landlen, IsKeyPressed(KEY_R));
	if ((won && IsKeyPressed(KEY_N)) || IsKeyPressed(KEY_R)) {
	    if (won) {
		FILE* file = fopen("lvlbak.txt", "a");
		fprintf(file, "%d", --nCrit);
		remove("lvl.txt");
		fclose(file);
		rename("lvlbak.txt", "lvl.txt");
		won = false;
	    }
	    
	    reset(
		map, garrx, garry,
		&landlen, land,
		&infelen, infectados,
		&drugAmt, &totalDrugs,
		nInfSpawn, nCrit,
		&dcenters );
	    //printf("%d %d\n", infelen, landlen);
	}
	//printf("%d << \n", infelen);
	
	DrawText(dw, GetScreenWidth() - 500, GetScreenHeight() - 100, 50, YELLOW);
	DrawText(sw, GetScreenWidth() - 500, 80, 50, YELLOW);
	free(dw);
	free(sw);

	if (totalDrugs >= 80.00) {
	    paused = true;
	    if (!won) PlaySound(clap);
	    won = true;
	    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 127 });
	    DrawTextCentered("Vitoria!", GetScreenWidth()/2,  GetScreenHeight()/2, 100, GREEN);
	    DrawTextCentered("Press \"n\" to continue", GetScreenWidth()/2, GetScreenHeight()/2 + 120, 40, GREEN);
	} if (dcenters == 0) {
	    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){ 0, 0, 0, 127 });
	    DrawTextCentered("Perdido!", GetScreenWidth()/2,  GetScreenHeight()/2, 100, RED);
	    DrawTextCentered("Press \"r\" to retry", GetScreenWidth()/2, GetScreenHeight()/2 + 120, 40, RED);
	}

	EndDrawing();
    }
}
