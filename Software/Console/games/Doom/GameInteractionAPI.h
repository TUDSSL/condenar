#ifndef GAMEINTERACTIONAPI
#define GAMEINTERACTIONAPI

#include "stdbool.h"

//Variables visible outside
extern float speedMultiplier; 
extern bool gamePaused;
extern bool godModeEnabled;
//End of variables visible outside



//Player ammo
int GetCurrentAmmo();
void SetCurrentAmmo(int amount);
void SetAmmoDecayRate(int rate);
//End of player ammo



//Player health
int GetCurrentHealth();
void SetCurrentHealth(int health);
void SetHealthDecayRate(int rate);
//End of player health



//Game speed
void SetGameSpeedMultiplier(float multiplier);
float GetGameSpeedMultipier();

void PauseGame();
void ResumeGame();
//End of game speed



//God mode
void SetGodModeEnabled(bool enabled);
bool GetGodModeEnabled();
//End of god mode



//APItick (This is run AFTER each game tick)
void DoAPITick();
//End of APItick


#endif