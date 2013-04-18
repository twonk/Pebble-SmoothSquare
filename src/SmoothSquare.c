/* Thanks for taking a look at the SmoothSquare source
   Full Disclosure: I havent worked with C in a number of years
   Im probably doing a lot of things wrong.  That said, the watch
   face seems to work pretty well, and hasnt caused any crashes
   on my pebble yet.
   Enjoy */
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x32, 0x33, 0x5C, 0x37, 0x71, 0x83, 0x4F, 0xD0, 0x85, 0x4B, 0xFA, 0xCE, 0x19, 0x37, 0x22, 0xBC }
PBL_APP_INFO(MY_UUID,
             "SmoothSquare", "Andrew Holmes",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define LAYERSIZE_X 36
#define LAYERSIZE_Y 48
#define OFFSCREEN  -60
#define CENTERPOSITION 54
#define ANIMATION_DURATION 500
#define DIGIT_DELAY 100
#define DATE_FORMAT "%A\n%e %b %Y"

Window window;

//Four text layers, one for each digit
TextLayer text_layer[4];
//The : gets a layer all of its own
TextLayer text_colon_layer;
//So does the date.
TextLayer text_date_layer;

//Four animations, one for each digit to move it up
PropertyAnimation prop_animation_up[4];
//And four more to drop each digit down
PropertyAnimation prop_animation_down[4];

static char date_text[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
//This is where we put the time
static char time_text[] = "0000";
//And we use this to see if the time has changed
static char time_text_was[] = "0000";

//Used to work out which digit has been pushed off the top of the screen
static int digitindex[4] = {0,1,2,3};

//Each digit gets its own array
static char time0[] = "0";
static char time1[] = "0";
static char time2[] = "0";
static char time3[] = "0";

char *format = "%I%M";

//Takes a digit as parameter and, works out where it should 
//drop to, and then schedules the animation.
void dropDigit(int digit){
    //We need to finess the x position sometimes
    int adjust_x = 0;

    switch (digit)
    {
      case 0:
        time0[0] = time_text[0];
        if(time0[0]=='0')time0[0]=' ';
        text_layer_set_text(&text_layer[0],time0);
        break;
      case 1:
        time1[0] = time_text[1];
        adjust_x = -6;
        if(time_text[0]=='0')adjust_x=-18;
        text_layer_set_text(&text_layer[1],time1);
        break;
      case 2:
        time2[0] = time_text[2];
        text_layer_set_text(&text_layer[2],time2);
        adjust_x = 6;
        break;
      case 3:
        time3[0] = time_text[3];
        text_layer_set_text(&text_layer[3],time3);
        break;
   };

    GRect to_rect = GRect((digit*LAYERSIZE_X)+adjust_x,CENTERPOSITION,LAYERSIZE_X,LAYERSIZE_Y);
    property_animation_init_layer_frame(&prop_animation_down[digit],&text_layer[digit].layer,NULL, &to_rect);
    animation_set_curve(&prop_animation_down[digit].animation, AnimationCurveEaseOut);
    animation_set_duration(&prop_animation_down[digit].animation,ANIMATION_DURATION);
    animation_set_delay(&prop_animation_down[digit].animation,(3-digit)*DIGIT_DELAY);
    animation_schedule(&prop_animation_down[digit].animation);
}

//Aninmation stopped is called when the push up off the screen has completed
//so we want to use this to make the digit drop back down
//The digit that we are animating has been pushed into the data parameter
void animation_stopped(Animation *animation, bool finished, void* data) {
  //The UI Framework documentation doesnt mention the bool finished
  //parameter which causes much upset if you dont have it and you
  //want to get at the data youve pushed in to the data parameter
  (void)finished;
  (void)animation;

  //dereference and cast the data to the corrrect type.
  int digit = *(int*)data;

  dropDigit(digit);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;

  //What time is it?  Pebble Time.
  string_format_time(time_text, sizeof(time_text), format, t->tick_time);

  //Loop through each of the digits
  int i;
  for (i=0; i<4; i++)
  {
    //Only update the digits that have changed values.
    if(time_text[i]!=time_text_was[i])
    {
      //Move the digit off the top of the screen.
      //animation_stopped handles bringing it back on.
      int adjust_x=0;
      if(i==1 && time_text_was[0]=='0')adjust_x=-18;
      if(i==1 && time_text_was[0]!='0')adjust_x=-6;
      if(i==2)adjust_x=6;

      GRect to_rect = GRect((i*LAYERSIZE_X)+adjust_x, OFFSCREEN,LAYERSIZE_X,LAYERSIZE_Y);
      property_animation_init_layer_frame(&prop_animation_up[i], &text_layer[i].layer,NULL, &to_rect);
      animation_set_curve(&prop_animation_up[i].animation , AnimationCurveEaseIn );
      animation_set_duration(&prop_animation_up[i].animation,ANIMATION_DURATION);
      animation_set_delay(&prop_animation_up[i].animation,(3-i)*DIGIT_DELAY);
      animation_set_handlers(&prop_animation_up[i].animation, (AnimationHandlers) {
         .stopped = (AnimationStoppedHandler) animation_stopped
      }, &(digitindex[i]) );
      //In the handlers statement above I need to pass the digit that has been
      //pushed off the screen. There is a data parameter, but I cant just put
      //the address of the loop variable in there because by the time that the
      //animation has stopped that address could have anything in it.
      //instead Im  pushing the address of a static array element which isnt going
      //to change.

      //Run the animation!
      animation_schedule(&prop_animation_up[i].animation);
    };
  };
  //Save the time
  string_format_time(time_text_was, sizeof(time_text_was), format, t->tick_time);

  //Set the date again in case it has changed.
  string_format_time(date_text, sizeof(date_text), DATE_FORMAT, t->tick_time);
  text_layer_set_text(&text_date_layer, date_text);

}


void handle_init(AppContextRef ctx) {
  (void)ctx;
  //Load up the resources so we can access our new font
  resource_init_current_app(&APP_RESOURCES);

  if(clock_is_24h_style()){
     format[1]='H';
  }
  else
  {
     format[1]='I';
  }

  window_init(&window, "Window Name");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window,GColorBlack);

  //get the current time and put it in to the time_text and time_text_was
  PblTm currentTime;
  get_time(&currentTime);
  string_format_time(time_text, sizeof(time_text), format, &currentTime);
  string_format_time(time_text_was, sizeof(time_text), format, &currentTime);

  //Setup the four text layers, one for each digit
  int i;
  for (i = 0; i<4; i++)
  {
    //Each digit starts off the top of the screen
    text_layer_init(&text_layer[i],GRect(i*LAYERSIZE_X,OFFSCREEN,LAYERSIZE_X,LAYERSIZE_Y));
    text_layer_set_font(&text_layer[i], fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MYFONT_34)));
    text_layer_set_text_color(&text_layer[i], GColorWhite);
    text_layer_set_background_color(&text_layer[i],GColorClear);
    text_layer_set_text_alignment(&text_layer[i],GTextAlignmentCenter);
    layer_add_child(&window.layer, &text_layer[i].layer);
    //Run the drop down to make the numbers appear on the screen
    dropDigit(i);
  };

  //Setup the date
  text_layer_init(&text_date_layer, GRect(8,126,144-8,168-126));
  text_layer_set_text_color(&text_date_layer, GColorWhite);
  text_layer_set_background_color(&text_date_layer, GColorClear);
  text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MYFONT_14)));
  layer_add_child(&window.layer, &text_date_layer.layer);
  string_format_time(date_text, sizeof(date_text), DATE_FORMAT, &currentTime);
  text_layer_set_text(&text_date_layer, date_text);

  //Setup the colon
  text_layer_init(&text_colon_layer, GRect(64,CENTERPOSITION-4,16,LAYERSIZE_Y));
  text_layer_set_text_color(&text_colon_layer, GColorWhite);
  text_layer_set_background_color(&text_colon_layer, GColorClear);
  text_layer_set_font(&text_colon_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MYFONT_34)));
  text_layer_set_text_alignment(&text_colon_layer,GTextAlignmentCenter);
  layer_add_child(&window.layer, &text_colon_layer.layer);
  text_layer_set_text(&text_colon_layer, ":");

}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
