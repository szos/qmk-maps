#include QMK_KEYBOARD_H
#include "unicode.h"

extern keymap_config_t keymap_config;


#define _QWERTY 0
#define _DVORAK 1
#define _CHORDS 2
#define _GAMING 3
#define _APL 4

#define _RATCON 5

#define MAX_LAYER_TRACKER 4

#define QWERTY TO(_QWERTY)
#define DVORAK TO(_DVORAK)
#define CHORDS TO(_CHORDS)
#define RATCON TO(_RATCON)

// define a function to cycle layers.
int track_layers_for_cycling = 0;
bool ratcon_is_on = false;
void cycle_layers(void) {
  if (track_layers_for_cycling == _QWERTY) {
    track_layers_for_cycling = _DVORAK;
    layer_on(track_layers_for_cycling);
  } else if (track_layers_for_cycling == _DVORAK) {
    layer_off(track_layers_for_cycling);
    track_layers_for_cycling = _CHORDS;
    layer_on(track_layers_for_cycling);
  } else if (track_layers_for_cycling == _CHORDS) {
    layer_off(track_layers_for_cycling);
    track_layers_for_cycling = _GAMING;
    layer_on(track_layers_for_cycling);
  } else if (track_layers_for_cycling == _GAMING) {
    layer_off(track_layers_for_cycling);
    track_layers_for_cycling = _APL;
    layer_on(track_layers_for_cycling);
  } else if (track_layers_for_cycling == _APL) {
    layer_off(track_layers_for_cycling);
    track_layers_for_cycling = 0;
    layer_on(track_layers_for_cycling);
  } else {
    track_layers_for_cycling = 0;
    layer_on(_QWERTY);
    layer_off(_DVORAK);
    layer_off(_CHORDS);
    layer_off(_GAMING);
    layer_off(_APL);
    layer_off(_RATCON);
  }
}

// define stuff for our custom chording setup.
int chord_value = 0;
int first_chord_press = 0;
bool chord_shift = false;
bool chord_shift_lock = false;
bool chord_control = false;
int chord_control_counter = 0;
bool chord_alt = false;
int chord_alt_counter = 0;
bool chord_ralt = false;
bool chord_ralt_lock = false;
bool chord_gui = false;
static uint16_t chord_combo_timer;

void matrix_init_user(void) {
  set_unicode_input_mode(UC_LNX);
};

// define predicates for checking whether shift, alt, ctrl, etc are pressed
#define shiftp get_mods() == MOD_BIT(KC_LSHIFT) || get_mods() == MOD_BIT(KC_RSHIFT)

#define laltp get_mods() == MOD_BIT(KC_LALT) // seperate lalt and ralt, cause ralt 
#define raltp get_mods() == MOD_BIT(KC_RALT) // is for alt green
#define altp get_mods() == MOD_BIT(KC_RALT) || get_mods() == MOD_BIT(KC_LALT)

#define ctlp get_mods() == MOD_BIT(KC_LCTL) || get_mods() == MOD_BIT(KC_RCTL)
// end predicates

void tap_key(uint16_t keycode){
  /*Function to 'tap' a key*/
  register_code(keycode);
  unregister_code(keycode);
};

void tap_key_16(uint16_t keycode){
  register_code16(keycode); 
  unregister_code16(keycode); 
}

void chord_key(uint16_t keycode) {
  if (chord_control_counter > 0) {
    chord_control_counter = chord_control_counter - 1;
    chord_control = true;
  }
  if (chord_alt_counter > 0) {
    chord_alt_counter = chord_alt_counter - 1;
    chord_alt = true;
  }
  if (chord_shift_lock) {
    chord_shift = true;
  }
  if (chord_ralt_lock) {
    chord_ralt = true;
  }
  if (chord_shift) {
    register_code(KC_LSFT);
  }
  if (chord_control) {
    register_code(KC_LCTL);
  }
  if (chord_alt) {
    register_code(KC_LALT);
  }
  if (chord_ralt) {
    register_code(KC_RALT);
  }
  if (chord_gui) {
    register_code(KC_LGUI);
  }
  register_code(keycode);
  unregister_code(keycode);
  if (chord_shift) {
    unregister_code(KC_LSFT);
    chord_shift = false;
  }
  if (chord_control) {
    unregister_code(KC_LCTL);
    chord_control = false;
  }
  if (chord_alt) {
    unregister_code(KC_LALT);
    chord_alt = false;
  }
  if (chord_ralt) {
    unregister_code(KC_RALT);
    chord_ralt = false;
  }
  if (chord_gui) {
    unregister_code(KC_LGUI);
    chord_gui = false;
  }
}

// define unicode sending functions:
void process_unicode_key_with_mods(int key, int sft_key, int lalt_key,
				   int ralt_key, int ctl_key, keyrecord_t *record)
{
  if ((shiftp) && !(altp || ctlp)){
    process_unicode((sft_key|QK_UNICODE), record);
  } else if (laltp && !(raltp || ctlp || shiftp)) {
    process_unicode((lalt_key|QK_UNICODE), record);
  } else if (raltp && !(laltp || ctlp || shiftp)) {
    process_unicode((ralt_key|QK_UNICODE), record);
  } else if ((ctlp) && !(altp || shiftp)) {
    process_unicode((ctl_key|QK_UNICODE), record);
  } else {
    process_unicode((key|QK_UNICODE), record);
  }
};

void process_unicode_key(int key,int shift_key, keyrecord_t *record)
{
  if (get_mods() == MOD_BIT(KC_LSHIFT) || get_mods() == MOD_BIT(KC_RSHIFT)) {
    process_unicode((shift_key|QK_UNICODE), record);
    return;
  }
  process_unicode((key|QK_UNICODE), record);
  return;
};

void process_1_unicode_key(int key, keyrecord_t *record)
{
  process_unicode((key|QK_UNICODE), record);
  return;
};
// end unicode functions. 

// Tap dance stuff goes here:
typedef struct {
  bool is_press_action;
  int state;
} tap;

enum {
//single S, double D, triple T, etc      
  S_TAP = 1,
  S_HOLD = 2,
  D_TAP = 3,
  D_HOLD = 4,
  D_S_TAP = 5,
  T_TAP = 6,
  T_HOLD = 7
};

int cur_dance(qk_tap_dance_state_t *state);
void x_finished(qk_tap_dance_state_t *state, void *user_data);
void x_reset(qk_tap_dance_state_t *state, void *user_data);

int cur_dance(qk_tap_dance_state_t *state) {
  if (state->count == 1){
    if (state->interrupted || !state->pressed)
      return S_TAP;
    else return S_HOLD;
  } else if (state->count == 2) {
    if (state->interrupted) return D_S_TAP;
    else if (state->pressed) return D_HOLD;
    else return D_TAP;
  }
  if (state->count == 3){
    if (state->interrupted || !state->pressed) return T_TAP;
    else return T_HOLD;
  }
  else return 8; // magic number, when doing more presses, dont use 8.
};

static tap xtap_state = {
  .is_press_action = true,
  .state = 0
};

void lctl_bsp_finished(qk_tap_dance_state_t *state, void *user_data) {
  xtap_state.state = cur_dance(state);
  switch (xtap_state.state) {
  case S_TAP: register_code(KC_BSPC); break;
  case S_HOLD: register_code(KC_LCTL); break;
  case D_HOLD: register_code(KC_BSPC);
  }
};

void lctl_bsp_reset(qk_tap_dance_state_t *state, void *user_data) {
  switch (xtap_state.state) {
  case S_TAP: unregister_code(KC_BSPC); break;
  case S_HOLD: unregister_code(KC_LCTL); break;
  case D_HOLD: unregister_code(KC_BSPC);
  }
  xtap_state.state = 0;
};

int cycle_layer_tracker = 0;

void cycle_layers_rat_hold_finished(qk_tap_dance_state_t *state, void *user_data) {
  xtap_state.state = cur_dance(state);
  switch (xtap_state.state) {
  case S_TAP:
    if (cycle_layer_tracker == _QWERTY){
      cycle_layer_tracker = cycle_layer_tracker + 1;
      layer_on(cycle_layer_tracker);
    } else if (cycle_layer_tracker == _DVORAK){
      layer_off(cycle_layer_tracker);
      cycle_layer_tracker = cycle_layer_tracker + 1;
      layer_on(cycle_layer_tracker);
    } else if (cycle_layer_tracker == _CHORDS){
      layer_off(cycle_layer_tracker);
      cycle_layer_tracker = cycle_layer_tracker + 1;
      layer_on(cycle_layer_tracker);
    } else if (cycle_layer_tracker == _APL){
      layer_off(cycle_layer_tracker);
      cycle_layer_tracker = 0;
    }
  case S_HOLD: layer_on(_RATCON);
  }
}; 

void cycle_layers_rat_hold_reset(qk_tap_dance_state_t *state, void *user_data) {
  switch (xtap_state.state) {
  case S_HOLD: layer_off(_RATCON);
  }
  xtap_state.state = 0;
};

int layer_dance_tracker_s = 0;
int layer_dance_tracker_d = 0;

void po_finished(qk_tap_dance_state_t *state, void *user_data) {
  xtap_state.state = cur_dance(state);
  switch (xtap_state.state) {
  case S_TAP: register_code16(LSFT(KC_8)); break;
  case S_HOLD: register_code(KC_LALT); break;
  case D_TAP: register_code16(LALT(KC_SPC)); break;
  case D_HOLD: register_code16(LSFT(KC_8));
  }
};

void po_reset(qk_tap_dance_state_t *state, void *user_data) {
  switch (xtap_state.state) {
  case S_TAP: unregister_code16(LSFT(KC_8)); break;
  case S_HOLD: unregister_code(KC_LALT); break;
  case D_TAP: unregister_code16(LALT(KC_SPC)); break;
  case D_HOLD: unregister_code16(LSFT(KC_8)); 
  }
  xtap_state.state = 0;
};

void pc_finished(qk_tap_dance_state_t *state, void *user_data) {
  xtap_state.state = cur_dance(state);
  switch (xtap_state.state) {
  case S_TAP: register_code16(LSFT(KC_9)); break;
  case S_HOLD: register_code(KC_LALT); break;
  case D_TAP: register_code16(LALT(KC_SPC)); break;
  case D_HOLD: register_code16(LSFT(KC_9));
  }
};

void pc_reset(qk_tap_dance_state_t *state, void *user_data) {
  switch (xtap_state.state) {
  case S_TAP: unregister_code16(LSFT(KC_9)); break;
  case S_HOLD: unregister_code(KC_LALT); break;
  case D_TAP: unregister_code16(LALT(KC_SPC)); break;
  case D_HOLD: unregister_code16(LSFT(KC_9)); 
  }
  xtap_state.state = 0;
};

void x_finished (qk_tap_dance_state_t *state, void *user_data) {
  /* example of sending x on a single tap, LCTL on a tap and hold, 
     ESC on double tap, LALT on double tap and hold, and double taps 
     x if you double tap and dont hold. 
   */
  xtap_state.state = cur_dance(state);
  switch (xtap_state.state) {
  case S_TAP: register_code(KC_X); break;
  case S_HOLD: register_code(KC_LCTRL); break;
  case D_TAP: register_code16(LSFT(KC_X)); break;
  case D_HOLD: register_code(KC_LALT); break;
  case D_S_TAP: register_code(KC_X); unregister_code(KC_X); register_code(KC_X);
    //Last case is for fast typing. Assuming your key is `f`:
    //For example, when typing the word `buffer`, and you want to make sure that you send `ff` and not `Esc`.
    //In order to type `ff` when typing fast, the next character will have to be hit within the `TAPPING_TERM`, which by default is 200ms.
  }
};

void x_reset (qk_tap_dance_state_t *state, void *user_data) {
  switch (xtap_state.state) {
    case S_TAP: unregister_code(KC_X); break;
    case S_HOLD: unregister_code(KC_LCTRL); break;
    case D_TAP: unregister_code16(LSFT(KC_X)); break;
    case D_HOLD: unregister_code(KC_LALT);
    case D_S_TAP: unregister_code(KC_X);
  }
  xtap_state.state = 0;
};

void ctl_met_finished(qk_tap_dance_state_t *state, void *user_data) {
  xtap_state.state = cur_dance(state);
  switch (xtap_state.state) {
  case S_HOLD: register_code16(LALT(KC_LCTL));
  }
};

void ctl_met_reset(qk_tap_dance_state_t *state, void *user_data) {
  switch (xtap_state.state) {
  case S_HOLD: unregister_code16(LALT(KC_LCTL));
  }
  xtap_state.state = 0;
};

enum {
      TAB_SHIFT_PFIX = 0,
      DANCE_2,
      LALT_PP,
      LA_PC,
      LCTL_BSPC,
      CTL_MET,
      CY_LY
};

qk_tap_dance_action_t tap_dance_actions[] = {
					     //  [DANCE_2] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, layer_dance_finished, layer_dance_reset),
  [LALT_PP] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, po_finished, po_reset),
  [LA_PC] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, pc_finished, pc_reset), 
  [LCTL_BSPC] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, lctl_bsp_finished, lctl_bsp_reset),
  [CTL_MET] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, ctl_met_finished, ctl_met_reset),
  [CY_LY] =  ACTION_TAP_DANCE_FN_ADVANCED(NULL, cycle_layers_rat_hold_finished, cycle_layers_rat_hold_reset)
};

// end tap dance stuff

// define keycodes within a safe range
enum my_keycodes {
  TESTER = SAFE_RANGE,
  U_RBC, U_LBC,
  R_ARW, L_ARW,
  // unicode keys
  // APL keys
  DIA_I,
  APL_1, APL_2, APL_3, APL_4, APL_5,
  APL_6, APL_7, APL_8, APL_9, APL_0,
  APL_DSH, APL_EQL,

  APL_Q, APL_W, APL_E, APL_R, APL_T, APL_Y,
  APL_U, APL_I, APL_O, APL_P, APL_LBC, APL_RBC,

  APL_A, APL_S, APL_D, APL_F, APL_G, APL_H,
  APL_J, APL_K, APL_L, APL_CLN, APL_QOT, //apl symbols on quote
  APL_QQT, //apl symbols on quasiquote

  APL_LGS, //apl less greater sign
  APL_Z, APL_X, APL_C, APL_V, APL_B, APL_N,
  APL_M, APL_CMA, APL_DOT, APL_FSH,
  
  // regular keys
  LPAREN, RPAREN, LSFT_PO, RSFT_PC, LALT_PO, LALT_PC,
  LSFT_BSP, /* swap bsp and tab on LSFT and LCTL */
  LSFT_TAB, LCTL_TAB, LCTL_BSP, RALT_DEL, RALT_LBC,
  RALT_RBC, LGUI_LBK, RGUI_RBK, RCTL_ENT, LS_TB_PX,

  LYR_CYC,
  // chording keys
  // CHORD_1 CHORD_2 CHORD_3 CHORD_4 CHORD_5
  KC_CD_1, KC_CD_2, KC_CD_3,
  KC_CD_4, KC_CD_5, KC_CD_C,
  // chording key mods
  CRD_CTL, CRD_SFT, CRD_ALT,
  CRD_RLT, CRD_GUI,
  // chording number keys
  CRD_1_6, CRD_2_7, CRD_3_8,
  CRD_4_9, CRD_5_0,
  // chording key punctuation
  CRD_DOT, CRD_COM, 
  // extra chording keys
  CRD_QOT, CRD_DSH, CRD_DED, CRD_SLH,
  CRD_AND, CRD_PER, CRD_OPN, CRD_CLS,
  CRD_EQL, CRD_RBC, CRD_NBS, CRD_NHS,
  CRD_EXL, CRD_GRV
};

/* enum combos { */
/*   // chord enums for definition. */
/*   // CH_1, */
/*   CH_12, */
/*   CH_123, */
/*   CH_1234, */
/*   CH_12345 */
/* }; */
// end keycode definitions

// combo definitions
// const uint16_t PROGMEM ch_12_combo[] = {CHORD_1, CHORD_2, COMBO_END};
// const uint16_t PROGMEM ch_123_combo[] = {CHORD_1, CHORD_2, CHORD_3, COMBO_END};
// const uint16_t PROGMEM ch__combo[] = {};
// end combo definitions

/* combo_t key_combos[COMBO_COUNT] = { */
/*   // [CH_12345] = COMBO(ch_12345_combo, KC_SPC)				    */
/* 				   [CH_12]    = COMBO(ch_12_combo, KC_A), */
/* 				   [CH_123]   = COMBO(ch_123_combo, KC_B) */
/*   // [CH_12345] = COMBO(ch_12345_combo, KC_SPC) */
/* } */;



// one key chords
#define CHORD_1 KC_P
#define CHORD_2 KC_H
#define CHORD_3 KC_D
#define CHORD_4 KC_B
#define CHORD_5 KC_A
// two key chords
#define CHORD_1_2 KC_X
#define CHORD_1_3 KC_T
#define CHORD_1_4 KC_R
#define CHORD_1_5 KC_Q
#define CHORD_2_3 KC_L
#define CHORD_2_4 KC_J
#define CHORD_2_5 KC_I
#define CHORD_3_4 KC_F
#define CHORD_3_5 KC_E
#define CHORD_4_5 KC_C
// three key chords
#define CHORD_1_2_3 KC_SCLN
#define CHORD_1_2_4 KC_Z
#define CHORD_1_2_5 KC_Y
#define CHORD_1_3_4 KC_V
#define CHORD_1_3_5 KC_U
#define CHORD_1_4_5 KC_S
#define CHORD_2_3_4 KC_N
#define CHORD_2_3_5 KC_M
#define CHORD_2_4_5 KC_K
#define CHORD_3_4_5 KC_G
// four key chords
#define CHORD_1_2_3_4 KC_SPC
#define CHORD_1_2_3_5 KC_LBRACKET
#define CHORD_1_2_4_5 KC_QUOT
#define CHORD_1_3_4_5 KC_W
#define CHORD_2_3_4_5 KC_O
// five key chord
#define CHORD_1_2_3_4_5 KC_BSPC

void process_chorded_key_value(int value) {
  if (value == 1) {
    chord_key(CHORD_1);
  } else if (value == 2) {
    chord_key(CHORD_2);
  } else if (value == 4) {
    chord_key(CHORD_3);
  } else if (value == 8) {
    chord_key(CHORD_4);
  } else if (value == 16) {
    chord_key(CHORD_5);
    /* END SINGLE CHORDS,
       BEGIN DOUBLE CHORDS */
  } else if (value == 3) {
    chord_key(CHORD_1_2);
  } else if (value == 5) {
    chord_key(CHORD_1_3);
  } else if (value == 9) {
    chord_key(CHORD_1_4);
  } else if (value == 17) {
    chord_key(CHORD_1_5);
  } else if (value == 6) {
    chord_key(CHORD_2_3);
  } else if (value == 10) {
    chord_key(CHORD_2_4);
  } else if (value == 18) {
    chord_key(CHORD_2_5);
  } else if (value == 12) {
    chord_key(CHORD_3_4);
  } else if (value == 20) {
    chord_key(CHORD_3_5);
  } else if (value == 24) {
    chord_key(CHORD_4_5);
    /* END DOUBLE CHORDS,
       BEGIN TRIPLE CHORDS */
  } else if (value == 7) {
    chord_key(CHORD_1_2_3);
  } else if (value == 11) {
    chord_key(CHORD_1_2_4);
  } else if (value == 19) {
    chord_key(CHORD_1_2_5);
  } else if (value == 13) {
    chord_key(CHORD_1_3_4);
  } else if (value == 21) {
    chord_key(CHORD_1_3_5);
  } else if (value == 25) {
    chord_key(CHORD_1_4_5);
  } else if (value == 14) {
    chord_key(CHORD_2_3_4);
  } else if (value == 22) {
    chord_key(CHORD_2_3_5);
  } else if (value == 26) {
    chord_key(CHORD_2_4_5);
  } else if (value == 28) {
    chord_key(CHORD_3_4_5);
    /* END TRIPLE CHORDS,
       BEGIN QUADRUPLE CHORDS */
  } else if (value == 15) {
    chord_key(CHORD_1_2_3_4);
  } else if (value == 23) {
    chord_key(CHORD_1_2_3_5);
  } else if (value == 27) {
    chord_key(CHORD_1_2_4_5);
  } else if (value == 29) {
    chord_key(CHORD_1_3_4_5);
  } else if (value == 30) {
    chord_key(CHORD_2_3_4_5);
  } else if (value == 31) {
    chord_key(CHORD_1_2_3_4_5);
  }
}

void matrix_scan_user(void){
  if (timer_elapsed(chord_combo_timer) >= CHORD_TIME && chord_value != 0) {
    process_chorded_key_value(chord_value);
    chord_value = 0;
    first_chord_press = 0;
  }
}

// CRD_CTL, CRD_SFT, CRD_ALT, 
// CRD_RLT, CRD_GUI - these are our chording modifiers (RLT is right alt)
/*
  we can use our custom shift lock with keys, even if were pressing shift on the mouse or keyboard!
  idk why, but it will let us use our shift lock as a switch, instead of a shift! then we can hit other keys alongside it!!
*/			 
// this is set up for norwegian key layout, so dont forget to `setxkbmap no`
bool process_record_user(uint16_t keycode, keyrecord_t *record){
  switch(keycode) {
  case LYR_CYC:
    if (record->event.pressed) {
      if (ctlp) {
        if (ratcon_is_on){
	  ratcon_is_on = false;
	  layer_off(_RATCON);
	} else {
	  ratcon_is_on = true;
	  layer_on(_RATCON);
	}
      } else {
	cycle_layers();
      }
    }
    return false;
  case CRD_GRV:
    if (record->event.pressed) {
      chord_key(KC_GRV);
    }
    return false;
  case CRD_NHS:
    if (record->event.pressed) {
      chord_key(KC_NUHS);
    }
    return false;
  case CRD_NBS:
    if (record->event.pressed) {
      chord_key(KC_NUBS);
    }
    return false;
  case CRD_EXL:
    if (record->event.pressed) {
      if ((chord_shift || chord_shift_lock) &&
	  !(chord_ralt || chord_ralt_lock)) {
	chord_key(KC_1);
      } else if ((chord_ralt || chord_ralt_lock) &&
		 !(chord_shift || chord_shift_lock)) {
	chord_shift = true;
	chord_key(KC_8);
      } else if ((chord_ralt || chord_ralt_lock) &&
		 (chord_shift || chord_shift_lock)) {
        chord_key(KC_9);
      } else {
	chord_shift = true;
	chord_key(KC_MINS);
      }
    }
    return false;
  case CRD_COM:
    if (record->event.pressed) {
      chord_key(KC_COMM);
    }
    return false;
  case CRD_DOT:
    if (record->event.pressed) {
      chord_key(KC_DOT);
    }
    return false;
  case CRD_DSH:
    if (record->event.pressed) {
      if ((chord_ralt || chord_ralt_lock) &&
	  !(chord_shift || chord_shift_lock)) {
        chord_ralt = false;
	chord_key(KC_MINS);
      } else {
	chord_key(KC_SLSH);
      }
    }
    return false;
  case CRD_DED:
    if (record->event.pressed) {
      chord_key(KC_RBRACKET);
    }
    return false;
  case CRD_SLH:
    if (record->event.pressed) {
      chord_key(KC_EQUAL);
    }
    return false;
  case CRD_AND:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_key(KC_3);
      } else {
	chord_shift = true;
	chord_key(KC_6);
      }
    }
    return false;
  case CRD_PER:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
        chord_ralt = true;
	chord_key(KC_5);
      } else {
	chord_shift = true;
	chord_key(KC_5);
      }
    }
    return false;
  case CRD_QOT:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_key(KC_2);
      } else {
  	chord_key(KC_NUHS);
      }
    }
    return false;
  case CRD_OPN:
    // sends opening paren/bracket/curly
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_shift = false;
	// chord_ralt = true;
	// chord_key(KC_8);
	register_code(KC_RALT);
	tap_key(KC_8);
        unregister_code(KC_RALT);
      } else if (chord_ralt || chord_ralt_lock) {
	chord_key(KC_7);
      } else {
	chord_shift = true;
	chord_key(KC_8);
      }
    }
    return false;
    // chord numbers
  case CRD_CLS:
    if (record->event.pressed) {
      if ((chord_ralt || chord_ralt_lock) && (chord_shift || chord_shift_lock)) {
	chord_ralt = false;
	chord_shift = false;
        register_code(KC_RALT);
	tap_key(KC_4);
        unregister_code(KC_RALT);
      }
      if (chord_shift || chord_shift_lock) {
	// this works! we should use this everywhere we use chord_shift
	// to decide what key to press.
	chord_shift = false;
	register_code(KC_RALT);
	tap_key(KC_9);
	unregister_code(KC_RALT);
      } else if (chord_ralt || chord_ralt_lock) {
	chord_key(KC_0);
      } else {
	chord_shift = true;
	chord_key(KC_9);
      }
    }
    return false;
  case CRD_EQL:
    if (record->event.pressed) {
      chord_key(KC_EQL);
    }
    return false;
  case CRD_RBC:
    if (record->event.pressed) {
      chord_key(KC_RBRC);
    }
    return false;
  case CRD_1_6:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_shift = false;
	tap_key(KC_6);
      } else {
	chord_key(KC_1);
      }
    }
    return false;
    // do more numbers!
  case CRD_2_7:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_shift = false;
	tap_key(KC_7);
      } else {
	chord_key(KC_2);
      }
    }
    return false;
  case CRD_3_8:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_shift = false;
	tap_key(KC_8);
      } else {
	chord_key(KC_3);
      }
    }
    return false;
  case CRD_4_9:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_shift = false;
	tap_key(KC_9);
      } else {
	chord_key(KC_4);
      }
    }
    return false;
  case CRD_5_0:
    if (record->event.pressed) {
      if (chord_shift || chord_shift_lock) {
	chord_shift = false;
	tap_key(KC_0);
      } else {
	chord_key(KC_5);
      }
    }
    return false;
    // chord punctuation
  case CRD_SFT:
    if (record->event.pressed) {
      if (chord_shift_lock) {
	// if caps is enabled
	chord_shift_lock = false;
      } else if (chord_shift) {
	// set caps on
	chord_shift_lock = true;
      } else {
	chord_shift = true;
      }
    }
    return false;
  case CRD_CTL:
    if (record->event.pressed) {
      chord_control = true;
      chord_control_counter = chord_control_counter + 1;
    }
    return false;
  case CRD_ALT:
    if (record->event.pressed) {
      chord_alt = true;
      chord_alt_counter = chord_alt_counter + 1;
    }
    return false;
  case CRD_RLT:
    if (record->event.pressed) {
      if (chord_ralt_lock) {
	// disable
	chord_ralt_lock = false;
      } else if (chord_ralt) {
	chord_ralt_lock = true;
      } else {
	chord_ralt = true;
      }
    }
    return false;
  case CRD_GUI:
    if (record->event.pressed) {
      chord_gui = true;
    }
    return false;
  case KC_CD_1:
    /* we have chord_value, first_chord_press, and chord_combo_timer globals
       which we can use and mess with. chord_value gets reset here, but
       first_chord_press and chord_value get set to 0 after we send the chorded
       key. 
    */
    if (record->event.pressed){
      if (first_chord_press == 0) {
	// then we start, and must set first_chord_press to 1
	first_chord_press = 1;
	chord_value = 0;
	chord_combo_timer = timer_read();
      }
      if (timer_elapsed(chord_combo_timer) < CHORD_TIME) {
	chord_value = chord_value + 1;
      }
    }
    return false;
  case KC_CD_2:
    /* we have chord_value, first_chord_press, and chord_combo_timer globals
       which we can use and mess with. chord_value gets reset here, but
       first_chord_press and chord_value get set to 0 after we send the chorded
       key. 
    */
    if (record->event.pressed){
      if (first_chord_press == 0) {
	// then we start, and must set first_chord_press to 1
	first_chord_press = 1;
	chord_value = 0;
	chord_combo_timer = timer_read();
      }
      if (timer_elapsed(chord_combo_timer) < CHORD_TIME) {
	chord_value = chord_value + 2;
      }
    }
    return false;
  case KC_CD_3:
    /* we have chord_value, first_chord_press, and chord_combo_timer globals
       which we can use and mess with. chord_value gets reset here, but
       first_chord_press and chord_value get set to 0 after we send the chorded
       key. 
    */
    if (record->event.pressed){
      if (first_chord_press == 0) {
	// then we start, and must set first_chord_press to 1
	first_chord_press = 1;
	chord_value = 0;
	chord_combo_timer = timer_read();
      }
      if (timer_elapsed(chord_combo_timer) < CHORD_TIME) {
	chord_value = chord_value + 4;
      }
    }
    return false;
  case KC_CD_4:
    /* we have chord_value, first_chord_press, and chord_combo_timer globals
       which we can use and mess with. chord_value gets reset here, but
       first_chord_press and chord_value get set to 0 after we send the chorded
       key. 
    */
    if (record->event.pressed){
      if (first_chord_press == 0) {
	// then we start, and must set first_chord_press to 1
	first_chord_press = 1;
	chord_value = 0;
	chord_combo_timer = timer_read();
      }
      if (timer_elapsed(chord_combo_timer) < CHORD_TIME) {
	chord_value = chord_value + 8;
      }
    }
    return false;
  case KC_CD_5:
    /* we have chord_value, first_chord_press, and chord_combo_timer globals
       which we can use and mess with. chord_value gets reset here, but
       first_chord_press and chord_value get set to 0 after we send the chorded
       key. 
    */
    if (record->event.pressed){
      if (first_chord_press == 0) {
	// then we start, and must set first_chord_press to 1
	first_chord_press = 1;
	chord_value = 0;
	chord_combo_timer = timer_read();
      }
      if (timer_elapsed(chord_combo_timer) < CHORD_TIME) {
	chord_value = chord_value + 16;
      }
    }
    return false;
  case U_RBC:
    if (record->event.pressed){
      process_unicode_key_with_mods(U_RIGHT_BRACE, U_RIGHT_CURLY, U_RIGHT_PAREN,
				    U_RIGHT_PAREN, U_RIGHT_PAREN, record);
    }
    return false;
  case L_ARW:
    if (record->event.pressed){
      process_unicode_key(U_LEFT_LESS_THAN, U_SECTION, record);
    }
    return false;
  case R_ARW:
    if (record->event.pressed){
      process_unicode_key(U_RIGHT_GREATER_THAN, U_PIPE, record);
    }
    return false;
  case TESTER:
    if (record->event.pressed){
      process_unicode((0x0028|QK_UNICODE), record);
    } 
    return false;
    
  case DIA_I:
    if (record->event.pressed) {
      process_unicode_key(0x00a8, 0x2336, record);
    }
    return false;
  case APL_2:
    if (record->event.pressed){
      process_unicode_key(U_MACRON, U_APL_FS_DOWN_TILDE, record);
    }
    return false;
  case APL_3:
    if (record->event.pressed){
      process_unicode_key(U_LESS, U_APL_FS_DOWN_STILE, record);
    }
    return false;
  case APL_4:
    if (record->event.pressed){
      process_unicode_key(U_LESS_EQUAL, U_APL_FS_DELTA_STILE, record);
    }
    return false;
  case APL_5:
    if (record->event.pressed){
      process_unicode_key(U_EQUAL, U_APL_FS_CIRCLE_STILE, record);
    }
    return false;
  case APL_6:
    if (record->event.pressed){
      process_unicode_key(U_GREATER_EQUAL, U_APL_FS_CIRCLE_BACKSLASH, record);
    }
    return false;
  case APL_7:
    if (record->event.pressed){
      process_unicode_key(U_GREATER, U_CIRCLE_MINUS, record);
    }
    return false;
  case APL_8:
    if (record->event.pressed){
      process_unicode_key(U_NOT_EQUAL, U_APL_FS_CIRCLE_STAR, record);
    }
    return false;
  case APL_9:
    if (record->event.pressed){
      process_unicode_key(U_LOG_OR, U_APL_FS_DOWN_CARET, record);
    }
    return false;
  case APL_0:
    if (record->event.pressed){
      process_unicode_key(U_LOG_AND, U_APL_FS_UP_CARET, record);
    }
    return false;
  case APL_DSH:
    if (record->event.pressed){
      process_unicode_key(U_MULTIPLICATION, U_EXCLAM, record);
    }
    return false;
  case APL_EQL:
    if (record->event.pressed){
      process_unicode_key(U_DIVIDE, U_APL_FS_QUAD_DIVIDE, record);
    }
    return false;
  case APL_Q:
    if (record->event.pressed){
      process_unicode_key(U_QUESTION_MARK, U_QUESTION_MARK, record);
    }
    return false;
  case APL_W:
    if (record->event.pressed){
      process_unicode_key(U_APL_OMEGA, U_APL_OMEGA, record);
    }
    return false;
  case APL_E:
    if (record->event.pressed){
      process_unicode_key(U_EPSILON, U_EPSILON_BAR, record);
    }
    return false;
  case APL_R:
    if (record->event.pressed){
      process_unicode_key(U_APL_RHO, U_APL_RHO, record);
    }
    return false;
  case APL_T:
    if (record->event.pressed){
      process_unicode_key(U_TILDE, U_APL_FS_ZILDE, record);
    }
    return false;
  case APL_Y:
    if (record->event.pressed){
      process_unicode_key(U_UP_ARROW, U_APL_FS_QUAD_UP_ARROW, record);
    }
    return false;
  case APL_U:
    if (record->event.pressed){
      process_unicode_key(U_DOWN_ARROW, U_APL_FS_QUAD_DOWN_ARROW, record);
    }
    return false;
  case APL_I:
    if (record->event.pressed){
      process_unicode_key(U_APL_IOTA, U_APL_IOTA_BAR, record);
    }
    return false;
  case APL_O:
    if (record->event.pressed){
      process_unicode_key(U_APL_CIRCLE, U_APL_QUAD, record);
    }
    return false;
  case APL_P:
    if (record->event.pressed){
      process_unicode_key(U_APL_STAR, U_APL_STAR_DIAERESIS, record);
    }
    return false;
  case APL_LBC:
    if (record->event.pressed){
      process_unicode_key(U_LEFT_ARROW, U_APL_FS_QUAD_LEFT_ARROW, record);
    }
    return false;
  case APL_RBC:
    if (record->event.pressed){
      process_unicode_key(U_RIGHT_ARROW, U_APL_FS_QUAD_RIGHT_ARROW, record);
    }
    return false;
    // new row (a)
  case APL_A:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_ALPHA, U_APL_FS_ALPHA_BAR, record);
    }
    return false;
  case APL_S:
    if (record->event.pressed){
      process_1_unicode_key(U_LEFT_CEILING, record);
    }
    return false;
  case APL_D:
    if (record->event.pressed){
      process_1_unicode_key(U_LEFT_FLOOR, record);
    }
    return false;
  case APL_F:
    if (record->event.pressed){
      process_1_unicode_key(U_APL_UNDERBAR, record);
    }
    return false;
  case APL_G:
    if (record->event.pressed){

      process_1_unicode_key(U_APL_NABLA, record);
    }
    return false;
  case APL_H:
    if (record->event.pressed){
      process_unicode_key(U_APL_DELTA, U_APL_FS_DELTA_UNDERBAR, record);
    }
    return false;
  case APL_J:
    if (record->event.pressed){
      process_unicode_key(U_APL_RING_OPERATOR, U_APL_FS_CIRCLE_DIAERESIS, record);
    }
    return false;
  case APL_K:
    if (record->event.pressed){
      process_unicode_key(U_APOSTROPHE, U_APL_FS_QUAD_EQUAL, record);
    }
    return false;
  case APL_L:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_QUAD, U_APL_FS_SQUISH_QUAD, record);
    }
    return false;
  case APL_CLN:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_DOWN_TACK_JOT, U_APL_IDENTICAL_TO, record);
    }
    return false;
  case APL_QOT:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_UP_TACK_JOT, U_APL_NOT_IDENTICAL_TO, record);
    }
    return false;
  case APL_QQT:
    if (record->event.pressed){
      process_unicode_key(U_DIAMOND_OPERATOR, U_APL_FS_QUAD_DIAMOND, record);
    }
    return false;
    // new row
  case APL_LGS:
    if (record->event.pressed){
      process_unicode_key(U_APL_RIGHT_TACK, U_APL_LEFT_TACK, record);
    }
    return false;
  case APL_Z:
    if (record->event.pressed){
      process_unicode_key(U_SUBSET_OF, U_SUBSET_OF_BAR, record);
    }
    return false;
  case APL_X:
    if (record->event.pressed){
      process_1_unicode_key(U_SUPERSET_OF, record);
    }
    return false;
  case APL_C:
    if (record->event.pressed){
      process_1_unicode_key(U_INTERSECTION, record);
    }
    return false;
  case APL_V:
    if (record->event.pressed){
      process_1_unicode_key(U_UNION, record);
    }
    return false;
  case APL_B:
    if (record->event.pressed){
      process_1_unicode_key(U_APL_DOWN_TACK, record);
    }
    return false;
  case APL_N:
    if (record->event.pressed){
      process_1_unicode_key(U_APL_UP_TACK, record);
    }
    return false;
  case APL_M:
    if (record->event.pressed){
      process_1_unicode_key(U_APL_DIVIDEES, record);
    }
    return false;
  case APL_CMA:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_COMMENT, U_APL_FS_COMMA_BAR, record);
    }
    return false;
  case APL_DOT:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_BACKSLASH_BAR, U_APL_FS_DELTA_UNDERBAR, record);
    }
    return false;
  case APL_FSH:
    if (record->event.pressed){
      process_unicode_key(U_APL_FS_SLASH_BAR, U_APL_FS_QUAD_COLON, record);
    }
    return false;
  case LSFT_PO:
    perform_space_cadet(record, KC_LSFT, KC_LSFT, KC_8);
    return false;
  case RSFT_PC:
    perform_space_cadet(record, KC_RSFT, KC_RSFT, KC_9);
    return false;
  case LALT_PO:
    perform_space_cadet(record, KC_LALT, KC_LSFT, KC_8);
    return false;
  case LALT_PC:
    perform_space_cadet(record, KC_LALT, KC_LSFT, KC_9);
    return false;
  case LSFT_BSP:
    perform_space_cadet(record, KC_LSFT, KC_TRNS, KC_BSPC);
    return false;
  case LSFT_TAB:
    perform_space_cadet(record, KC_LSFT, KC_TRNS, KC_TAB);
    return false;
  case LCTL_TAB:
    perform_space_cadet(record, KC_LCTL, KC_TRNS, KC_TAB);
    return false;
  case LCTL_BSP:
    perform_space_cadet(record, KC_LCTL, KC_TRNS, KC_BSPC);
    return false;
  case RALT_DEL:
    perform_space_cadet(record, KC_RALT, KC_TRNS, KC_DEL);
    return false;
  case RALT_LBC:
    perform_space_cadet(record, KC_RALT, KC_RALT, KC_7);
    return false;
  case RALT_RBC:
    perform_space_cadet(record, KC_RALT, KC_RALT, KC_0);
    return false;
  case LGUI_LBK:
    perform_space_cadet(record, KC_LGUI, KC_RALT, KC_8);
    return false;
  case RGUI_RBK:
    perform_space_cadet(record, KC_RGUI, KC_RALT, KC_9);
    return false;
  case RCTL_ENT:
    perform_space_cadet(record, KC_RCTL, KC_TRNS, KC_ENT);
    return false;
  case LS_TB_PX:
    // perform_space_cadet(record, KC_LSFT, KC_TRNS, TAP_DANCE_1);
    return false;
  case LPAREN:
    if (record->event.pressed){
      process_unicode((0x0028|QK_UNICODE), record);
    }
    return false;
  case RPAREN:
    if (record->event.pressed){
      process_unicode((0x0029|QK_UNICODE), record);
      return true;
    }
    return false;
  default:
    return true;
  }
};
// end process recording

// define key names to be used in the keymap
#define RALT_RET RALT_T(KC_ENT)
#define LALT_RET LALT_T(KC_ENT)
#define F_SFT SFT_T(KC_F)
#define J_SFT SFT_T(KC_J)
#define SPC_CTL RCTL_T(KC_SPC)
#define DEL_CTL LCTL_T(KC_DEL)
#define SPC_SFT SFT_T(KC_SPC)

#define RALT_PO RALT_T(KC_LPRN)
#define RALT_PC RALT_T(KC_RPRN)
#define RALT_TB RALT_T(KC_TAB)

#define RCTL_RET RCTL_T(KC_ENT)
#define RCTL_BKS RCTL_T(KC_BSPC)
#define RSFT_SPC SFT_T(KC_SPC)
#define LCTL_BKS LCTL_T(KC_BSPC)
#define RALT_BKS RALT_T(KC_BSPC)
#define LALT_BKS LALT_T(KC_BSPC)

#define SFT_PO_U SFT_T(LPAREN)
#define LCTL_PC_U LCTL_T(RPAREN)

#define SFT_PO_U_L SFT_T(UC(0x3F))
#define LCTL_PC_U_L LCTL_T(UC(0x2A))

#define TB_SFT SFT_T(KC_TAB)
#define BS_SFT SFT_T(KC_BSPC)

#define D_LYR LT(_SYMS, KC_D)
#define K_LYR LT(_SYMS, KC_K)
// end key naming for keymap

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  [_QWERTY] = LAYOUT_6x6(

     KC_F1  , KC_F2 , KC_F3 , KC_F4 , KC_F5 , KC_F6 ,                         KC_F7 , KC_F8 , KC_F9 ,KC_F10 ,KC_F11 ,KC_F12 ,
     KC_ESC , KC_1  , KC_2  , KC_3  , KC_4  , KC_5  ,                         KC_6  , KC_7  , KC_8  , KC_9  , KC_0  ,KC_BSLASH,
     KC_GRV , KC_Q  , KC_W  , KC_E  , KC_R  , KC_T  ,                         KC_Y  , KC_U  , KC_I  , KC_O  , KC_P  ,KC_LBRACKET,
     KC_TAB , KC_A  , KC_S  , KC_D  , KC_F  , KC_G  ,                         KC_H  , KC_J  , KC_K  , KC_L  ,KC_SCLN,KC_QUOT,
     LYR_CYC, KC_Z  , KC_X  , KC_C  , KC_V  , KC_B  ,                         KC_N  , KC_M  ,KC_COMM,KC_DOT ,KC_SLSH,KC_RBRACKET,
                     KC_NUBS,KC_NUHS,                                                        KC_MINS,KC_EQUAL,
                                      LSFT_TAB,LCTL_BKS,                       RCTL_ENT,RSFT_SPC,
//                                    LSFT_TAB,LCTL_BSP,                       RCTL_SPC,RSFT_ENT, 
                                      RALT_LBC,LALT_PO,                        LALT_PC,RALT_RBC,
     //                                  TD(CTL_MET),LGUI_LBK,                       RGUI_RBK, TD(CTL_MET
              MT(MOD_LCTL | MOD_LALT, KC_INS),LGUI_LBK,                       RGUI_RBK,MT(MOD_LCTL | MOD_LALT, KC_DEL)
  ),

								
  [_DVORAK] = LAYOUT_6x6(

     KC_F1  , KC_F2 , KC_F3 , KC_F4 , KC_F5 , KC_F6 ,                         KC_F7 , KC_F8 , KC_F9 ,KC_F10 ,KC_F11 ,KC_F12 ,     
     KC_ESC , KC_1  , KC_2  , KC_3  , KC_4  , KC_5  ,                         KC_6  , KC_7  , KC_8  , KC_9  , KC_0  ,KC_GRV ,
     KC_GRV ,KC_LBRC,KC_COMM,KC_DOT , KC_P  , KC_Y  ,                         KC_F  , KC_G  , KC_C  , KC_R  , KC_L  ,KC_BSLASH,
     KC_SCLN, KC_A  , KC_O  , KC_E  , KC_U  , KC_I  ,                         KC_D  , KC_H  , KC_T  , KC_N  , KC_S  ,KC_SLSH,
     LYR_CYC,KC_QUOT, KC_Q  , KC_J  , KC_K  , KC_X  ,                         KC_B  , KC_M  , KC_W  , KC_V  , KC_Z  ,KC_RBRACKET,
                     KC_NUBS,KC_SLSH,                                                        KC_MINS,KC_EQUAL,
                                    LSFT_TAB,LCTL_BKS,                      RCTL_ENT,RSFT_SPC,
                                    RALT_LBC,LALT_PO,                        LALT_PC,RALT_RBC,
     //                                 TD(CTL_MET),LGUI_LBK,                      RGUI_RBK, TD(CTL_MET)
             MT(MOD_LCTL | MOD_LALT, KC_INS),LGUI_LBK,                       RGUI_RBK,MT(MOD_LCTL | MOD_LALT, KC_DEL)
     /*
       Ok, so, what we want is a tap dance on the left hand thumb cluster where  
       tap; go to some layer x
       press+hold; momentary to layer y
       tap tap; go layer a 
       tap press+hold; momentary layer b
       tap tap tap; go to layer i
       tap tap press+hold; momentary layer j
      */
  ),

  [_CHORDS] = LAYOUT_6x6(
			 /*
			   we can use our custom shift lock with keys, even if were pressing shift on the mouse or keyboard!
			   idk why, but it will let us use our shift lock as a switch, instead of a shift! then we can hit other keys alongside it!!
			  */			 

     KC_F1  , KC_F2 , KC_F3 , KC_F4 , KC_F5 , KC_F6 ,                         KC_F7  , KC_F8 , KC_F9 ,KC_F10 ,KC_F11 ,KC_F12 ,
     KC_ESC ,CRD_1_6,CRD_2_7,CRD_3_8,CRD_4_9,CRD_5_0,                         CRD_1_6,CRD_2_7,CRD_3_8,CRD_4_9,CRD_5_0,KC_ESC,
     CRD_RBC,CRD_CLS,CRD_COM,CRD_ALT,CRD_RLT,CRD_EXL,                         CRD_EXL,CRD_RLT,CRD_ALT,CRD_COM,CRD_CLS,CRD_RBC,
     CRD_EQL,KC_CD_1,KC_CD_2,KC_CD_3,KC_CD_4,CRD_DSH,                         CRD_DSH,KC_CD_4,KC_CD_3,KC_CD_2,KC_CD_1,CRD_EQL,
     LYR_CYC,CRD_OPN,CRD_DOT,CRD_CTL,CRD_SFT,CRD_QOT,                         CRD_QOT,CRD_SFT,CRD_CTL,CRD_DOT,CRD_OPN,KC_BSLASH,
                     CRD_NBS,CRD_NHS,                                                      CRD_NHS,CRD_NBS,
                                      KC_CD_5,KC_ENT,                         KC_CD_5,KC_ENT,
                                      KC_TAB , KC_BSPC,                       KC_BSPC,KC_TAB,
                                      CRD_GRV,CRD_GUI,                        CRD_GUI,CRD_GRV
  ),


  [_GAMING] = LAYOUT_6x6(

     KC_ESC , KC_F2 , KC_F3 , KC_F4 , KC_F5 , KC_6  ,                         KC_F7 , KC_F8 , KC_F9 ,KC_F10 ,KC_F11 ,KC_F12 ,
     KC_N   , KC_1  , KC_2  , KC_3  , KC_4  , KC_5  ,                         KC_6  , KC_7  , KC_8  , KC_9  , KC_0  ,KC_BSPC,
     KC_P   , KC_Y  , KC_Q  , KC_W  , KC_E  , KC_R  ,                         KC_Y  , KC_U  , KC_I  , KC_O  , KC_P  ,KC_MINS,
     KC_I   ,KC_LSFT, KC_A  , KC_S  , KC_D  , KC_F  ,                         KC_H  , KC_J  , KC_K  , KC_L  ,KC_SCLN,KC_QUOT,
     LYR_CYC, KC_9  , KC_Z  , KC_X  , KC_X  , KC_V  ,                         KC_N  , KC_M  ,KC_COMM,KC_DOT ,KC_SLSH,KC_BSLASH,
                      KC_0  , KC_9  ,                                                       KC_PLUS, KC_EQL,
                                      KC_LCTL,KC_SPC,                         KC_ENT, KC_BSPC,
                                      KC_G  ,  KC_L ,                         KC_END,  KC_DEL,
                                      KC_B , KC_GRV,                        KC_LGUI, KC_LALT
  ),

    [_APL] = LAYOUT_6x6(

       KC_ESC , KC_1  , KC_2  , KC_3  , KC_4  , KC_5  ,                         KC_6  , KC_7  , KC_8  , KC_9  , KC_0  ,_______,
       APL_QQT, DIA_I , APL_2 , APL_3 , APL_4 , APL_5 ,                         APL_6 , APL_7 , APL_8 , APL_9 , APL_0 ,APL_DSH,
       _______, APL_Q , APL_W , APL_E , APL_R , APL_T ,                         APL_Y , APL_U , APL_I , APL_O , APL_P ,APL_EQL,
       APL_LGS, APL_A , APL_S , APL_D , APL_F , APL_G ,                         APL_H , APL_J , APL_K , APL_L ,APL_CLN,APL_QOT,
       LYR_CYC, APL_Z , APL_X , APL_C , APL_V , APL_B ,                         APL_N , APL_M ,APL_CMA,APL_DOT,APL_FSH,_______,
                       _______,_______,                                                        APL_LBC,APL_RBC,
                                               _______,_______,            _______,_______,
                                               _______,_______,            _______,_______,
                                               _______,_______,            _______,_______		      
		      
  ),
  
  [_RATCON] = LAYOUT_6x6(
     _______,_______,_______,_______,_______,_______,                       _______,_______,_______,_______,_______,_______,
     _______,_______,_______,_______,_______,_______,                       _______,_______,_______,_______,_______,_______,
     _______,_______,_______,_______,_______,_______,                       _______,_______,KC_MS_U,_______,_______,_______,
     _______,_______,_______,_______,_______,_______,                       _______,KC_MS_L,KC_MS_D,KC_MS_R,_______,_______,
     _______,_______,_______,_______,_______,_______,                       _______,KC_WH_D,KC_WH_U,KC_WH_R,KC_WH_L,_______,
                     _______,_______,                                                       _______,_______,
                                     _______,_______,                       KC_BTN1,KC_BTN2,
                                     _______,_______,                       _______,_______,
                                     _______,_______,                       _______,_______
    ),

};
