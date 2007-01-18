
/* Macros to check fc IO */
#define CAMPAN_FC1 (GPLR(btweb_features.fc_pan) & GPIO_bit(btweb_features.fc_pan))
#define CAMPAN_FC2 (GPLR(btweb_features.fc2_pan) & GPIO_bit(btweb_features.fc2_pan))
#define CAMTILT_FC1 (GPLR(btweb_features.fc_tilt) & GPIO_bit(btweb_features.fc_tilt))
#define CAMTILT_FC2 (GPLR(btweb_features.fc2_tilt) & GPIO_bit(btweb_features.fc2_tilt))

void init_cammotors(void);
void cam_panfwd(void);
void cam_panback(void);
void cam_panstop(void);
void cam_tiltfwd(void);
void cam_tiltback(void);
void cam_tiltstop(void);
