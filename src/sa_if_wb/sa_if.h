#ifndef _SA_IF_H__
#define _SA_IF_H__

typedef struct
{
  int r,g,b;
} color_t;

typedef struct
{
  char Text[255];
  color_t  Color;
} status_t;




///////////////////////////////////////////// FUNCTION PROTOTYPES ///////////////////////////////

void GetConfigParam(char *, char *, char *);
void SetConfigParam(char *, char *, char *);
void ReadSavedParams();
void do_snapcheck();
int IsImageToBeChanged(int x,int y);
//void Keyboard(char *, char *, int);
int openTouchScreen(int);
int getTouchScreenDetails(int*, int* ,int* ,int*);
void TransformTouchMap(int x, int y);
int IsButtonPushed(int NbButton, int x, int y);
int IsMenuButtonPushed(int x, int y);
int InitialiseButtons();
int AddButton(int x, int y, int w, int h);
int ButtonNumber(int, int);
int CreateButton(int MenuIndex, int ButtonPosition);
int AddButtonStatus(int ButtonIndex,char *Text,color_t *Color);
void AmendButtonStatus(int ButtonIndex, int ButtonStatusIndex, char *Text, color_t *Color);
void DrawButton(int ButtonIndex);
void SetButtonStatus(int ,int);
int GetButtonStatus(int ButtonIndex);
int getTouchSample(int*, int*, int*);
void UpdateWindow();
void wait_touch();
void CalculateMarkers();
void Normalise();
void ChangeLabel(int);
void ChangeRange(int button);
void ChangeSensor(int button);
void *WaitButtonEvent(void * arg);
void Define_Menu1();                 // XY Main Menu
void Define_Menu2();                 // XY Marker Menu
void Define_Menu3();                 // XY Labels Menu
void Define_Menu4();                 // XY System Menu
void Define_Menu5();                 // Mode Menu
void Define_Menu6();                 // Meter (Main) Menu
void Define_Menu7();
void Define_Menu8();
void Define_Menu9();
void Start_Highlights_Menu1();
void Start_Highlights_Menu4();
void Start_Highlights_Menu5();
void Start_Highlights_Menu6();
void Start_Highlights_Menu8();
void Start_Highlights_Menu9();
void Define_Menu41();
void DrawEmptyScreen();
void DrawYaxisLabels();
void DrawSettings();
void DrawTrace(int xoffset, int prev2, int prev1, int current);
void DrawMeterBox();
void DrawMeterArc();
void DrawMeterTicks(int major_ticks, int minor_ticks);
void DrawMeterSettings();
void Draw5MeterLabels(float LH_Value, float RH_Value);
void *MeterMovement(void * arg);
void ShowdBm(float dBm);
void ShowmW(float mW);
void Showraw(int);
void Showvolts(float);
static void cleanexit(int calling_exit_code);
static void terminate(int dummy);


typedef struct
{
  int adc_val;
  float pwr_dBm;
} adc_lookup;



#endif /* _SA_IF_H__ */
