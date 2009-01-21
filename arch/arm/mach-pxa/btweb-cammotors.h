
/* Macros to check fc IO */
#define CAMPAN_FC1 (GPLR(btweb_features.fc_pan) & GPIO_bit(btweb_features.fc_pan))
#define CAMPAN_FC2 (GPLR(btweb_features.fc2_pan) & GPIO_bit(btweb_features.fc2_pan))
#define CAMTILT_FC1 (GPLR(btweb_features.fc_tilt) & GPIO_bit(btweb_features.fc_tilt))
#define CAMTILT_FC2 (GPLR(btweb_features.fc2_tilt) & GPIO_bit(btweb_features.fc2_tilt))

/* Default Step frequency in Hz */
#define CAM_HZ 500

/* Max and min allowed stepping frequency */
#define CAM_HZ_MAX 2000
#define CAM_HZ_MIN   50

void init_cammotors(void);
void cam_sethz(int);
int cam_gethz(void);
void cam_panfwd(void);
void cam_panback(void);
int  cam_panstop(void);
void cam_tiltfwd(void);
void cam_tiltback(void);
int cam_tiltstop(void);
int cam_panmove(int);
int cam_tiltmove(int);
int cam_getpan(void);
int cam_gettilt(void);

