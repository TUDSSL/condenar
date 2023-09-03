#include "GameInteractionAPI.h"
#include "Helpers/Misc.h"
#include "Helpers/TimeKeeper.h"
#include "i_video.h"
#include "Helpers/PinDefinitions.h"
#include "Kernel.h"

#include "global_data.h"

int decayRate_health=0;
int decayRate_ammo=0;
float speedMultiplier = 1; 
bool gamePaused=false;
bool godModeEnabled=false;

int lastSecondMicros=0;

bool doomBeingCranked = false;

int GetCurrentAmmo(){
    player_t *plyr = &_g->player;

    if(plyr == NULL) return 0;

    //No weapon selected?
    if(plyr->readyweapon < 0 ||plyr->readyweapon > NUMWEAPONS) return 0;

    ammotype_t ammo = weaponinfo[plyr->readyweapon].ammo;   

    return plyr->ammo[ammo];
}

void SetCurrentAmmo(int amount){
    player_t *plyr = &_g->player;

    if(plyr == NULL) return;

    //No weapon selected?
    if(plyr->readyweapon < 0 ||plyr->readyweapon > NUMWEAPONS) return;

    ammotype_t ammo = weaponinfo[plyr->readyweapon].ammo;   

    plyr->ammo[ammo]=amount;
}

void SetAmmoDecayRate(int rate){
    decayRate_ammo=rate;
}




int GetCurrentHealth(){
    player_t *plyr = &_g->player;

    if(plyr == NULL) return 0;
    if(plyr->mo == NULL) return 0;

    return plyr->mo->health;
}
void SetCurrentHealth(int health){
    player_t *plyr = &_g->player;

    if(health>100) health=100;

    if(plyr == NULL) return;
    if(plyr->mo == NULL) return;

    plyr->mo->health=health;
    plyr->health = health;
}
void SetHealthDecayRate(int rate){
    decayRate_health=rate;
}



void SetGameSpeedMultiplier(float multiplier){
    speedMultiplier=multiplier;
}
float GetGameSpeedMultipier(){
    return speedMultiplier;
}

void PauseGame(){
    gamePaused=false;
}
void ResumeGame(){
    gamePaused=true;
}

CHECKPOINT_EXCLUDE_BSS
bool doom_buttonPausePressed = false;


void SetGodModeEnabled(bool enabled){
    godModeEnabled=enabled;
}
bool GetGodModeEnabled(){
    return godModeEnabled;
}

bool doom_chaingunOldOwned = false;
short doom_chaingunOldWeapon = -1;

//This is run AFTER each game tick
void DoAPITick(){

    if(micros()-lastSecondMicros>1000000 || micros()<lastSecondMicros){
        //Ammo decay rate
        //SetCurrentAmmo(GetCurrentAmmo()-decayRate_ammo);
        //if(GetCurrentAmmo()<0) SetCurrentAmmo(0);

        //Health decay rate
        SetCurrentHealth(GetCurrentHealth()-decayRate_health);
        if(GetCurrentHealth()<0) GetCurrentHealth(0);

        lastSecondMicros=micros();
    }    

    //Do other stuff after each tick here!
   /* if (GPIO_GetLevel(PIN_BUTTON5) && GPIO_GetLevel(PIN_BUTTON6))
    {       
        G_DeferedInitNew(1, 1, 6);
        //G_DoPlayDemo();
    }*/

    /*if(keysDown()==(KEY_LEFT)){
        SetAmmoDecayRate(-1);
    }else{
        SetAmmoDecayRate(0);
    }*/
    k_input_state input;
    k_GetInput(&input,false);

    if(k_IsBeingCranked() && k_GetSettingBool("/DOOM/Use crank",false)){
        doomBeingCranked = true;
        SetHealthDecayRate(-5);
            //No weapon selected?   

        player_t *plyr = &_g->player;

        if(plyr->readyweapon!=wp_chaingun && plyr->pendingweapon!=wp_chaingun){
            doom_chaingunOldWeapon = plyr->readyweapon;
            doom_chaingunOldOwned = plyr->weaponowned[wp_chaingun];
            plyr->weaponowned[wp_chaingun] = 1;     
            if(plyr->readyweapon!=wp_chaingun) plyr->pendingweapon = wp_chaingun;
            plyr->forceFire = true;
        }
        ammotype_t ammo = weaponinfo[wp_chaingun].ammo;   
        plyr->ammo[ammo]=51;
    }else{
        SetHealthDecayRate(0);
        doomBeingCranked = false;
        if(doom_chaingunOldWeapon!=-1){
            player_t *plyr = &_g->player;
            plyr->weaponowned[wp_chaingun] = false;
            plyr->pendingweapon = doom_chaingunOldWeapon;
            doom_chaingunOldWeapon = -1;
            plyr->forceFire = false;
        }
    } 

    if(input.buttonY != doom_buttonPausePressed && k_GetSettingBool("/Experiments/Pause enabled",true)){
        if(input.buttonY){
            _g->paused = !_g->paused;
        }
        doom_buttonPausePressed = input.buttonY;
    }

    if(input.buttonPower && input.buttonB){
        k_DelayMS(1000);    
        G_DeferedInitNew(1, 1, 9);
    }
    
    if(input.buttonPower && input.buttonX){
        k_DelayMS(1000);    
        G_DeferedInitNew(1, 1, 2);
    }
    

    /*if(godModeEnabled){       
        V_SetPalette(RADIATIONPAL);
        if(GetCurrentHealth()<1) SetCurrentHealth(1);
    }else{
        I_SetPalette(0);
    }

    LOG_I("Flash reads this frame: %d",flashReads);
    flashReads=0;*/
}

