#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include <alsa/control.h>
#include "LibInterface.h"

#define AUDIO_POWERSTAT_REG "/sys/class/codec/power_state"
#define A_SLEEP  1
#define A_ACTIVE 2

int volume_playback[101] = {
0,
2,
4,
5,
7,
9,
11,
12,
14,
16,
18,
19,
21,
23,
25,
26,
28,
30,
32,
33,
35,
37,
39,
40,
42,
44,
46,
47,
49,
51,
53,
54,
56,
58,
60,
61,
63,
65,
67,
68,
70,
72,
74,
75,
77,
79,
81,
82,
84,
86,
88,
89,
91,
93,
95,
96,
98,
100,
102,
103,
105,
107,
109,
110,
112,
114,
116,
117,
119,
121,
123,
124,
126,
128,
130,
131,
133,
135,
137,
138,
140,
142,
144,
145,
147,
149,
151,
152,
154,
156,
158,
159,
161,
163,
165,
166,
168,
170,
172,
173,
175
};

E_JATIN_ERROR_CODES SetAudioPowerMode(E_POWERMODE mode)
{
	char buf[30];
	char pre_stat = A_ACTIVE;
	memset(buf,0x00,sizeof(buf));
	ReadRegString(AUDIO_POWERSTAT_REG,buf,sizeof(buf));

	if(strstr(buf,"sleep") != NULL)
	{
		pre_stat = A_SLEEP;
		INFO_DEBUG("Previous stat is SLEEP \r\n");
	}

	if (mode == E_POWERMODE_IDLE && pre_stat != A_SLEEP)
	{
		ExecuteSytemCommand("/usr/sbin/alsactl -f /var/lib/alsa/asound_backup.state store", NULL);
		WriteRegString(AUDIO_POWERSTAT_REG,"sleep");

	}
	if ((mode == E_POWERMODE_ACTIVE) && pre_stat != A_ACTIVE)
	{
		WriteRegString(AUDIO_POWERSTAT_REG,"normal");
		ExecuteSytemCommand("/usr/sbin/alsactl -f /var/lib/alsa/asound_backup.state restore", NULL);
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetAudioJackStatus(E_AUDIO_JACK_CONNECTION *jack_status)
{

	int err;
	E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;
	snd_hctl_t *hctl;
	snd_ctl_elem_value_t *val;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_id_t *id;

	if ((err = snd_hctl_open(&hctl, "default", 0)) < 0) {
		INFO_ERROR("Control  open error: %s\n", snd_strerror(err));
		return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();
	if ((err = snd_hctl_load(hctl)) < 0) {
		INFO_ERROR("Control load error: %s\n", snd_strerror(err));
		snd_hctl_close(hctl);
		return E_JATIN_FAILURE;
	}
	snd_ctl_elem_value_alloca(&val);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_name(id, "Headphone Jack");
	elem = snd_hctl_find_elem(hctl, id);
	if (elem)
	{
		//	printf("Element found \r\n");
		snd_hctl_elem_read(elem, val);
		//	printf("@@@@@  JacK Status %s \r\n ", snd_ctl_elem_value_get_boolean(val, 0) ? "on" : "off");
		if(snd_ctl_elem_value_get_boolean(val, 0) > 0)
		{
			*jack_status = E_AUDIO_JACK_CONNECTED;
		} else {
			*jack_status = E_AUDIO_JACK_NOT_CONNECTED;
		}
		status = E_JATIN_SUCCESS;
	} else {
		status = E_JATIN_FAILURE;
		INFO_ERROR("Jack element Not found!! \r\n");
	}

	snd_hctl_close(hctl);
	snd_config_update_free_global();
	return status;
}

E_JATIN_ERROR_CODES alsa_mute_playback_control(int in_val)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "DAC Left Mute";
	int err = 0;
	E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;

	err = snd_mixer_open(&handle, 0);
	if(err < 0)
	{
		INFO_ERROR("Failed to open mixer %d %s",err,snd_strerror(err));
		//status = -1;
		return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

	if(handle)
	{
		do {
			err = snd_mixer_attach(handle, card);
			if(err < 0)
			{
				INFO_ERROR("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}

			err = snd_mixer_selem_register(handle, NULL, NULL);
			if(err < 0)
			{
				INFO_ERROR("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}

			err = snd_mixer_load(handle);
			if(err < 0)
			{
				INFO_ERROR("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}

			snd_mixer_selem_id_alloca(&sid);
			if(sid == NULL)
			{
				INFO_ERROR("Failed to alloc mixer element \r\n");
				status = E_JATIN_FAILURE;
				break;
			}

			snd_mixer_selem_id_set_index(sid, 0);
			snd_mixer_selem_id_set_name(sid, selem_name);
			snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
			if(elem == NULL)
			{
				INFO_ERROR("Requested mixer element not found \r\n");
				status = E_JATIN_FAILURE;
				break;
			}

			err = snd_mixer_selem_set_playback_switch_all(elem,in_val);
			if(err < 0)
			{
				INFO_ERROR("Failed to set mute switch val   %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			} else {
				status = E_JATIN_SUCCESS;
			}
		} while(0);
		/*
		snd_mixer_selem_get_playback_switch(elem,0,&in_val);
		printf("get capture mute switch %d \r\n",in_val);
		*/

		err = snd_mixer_close(handle);
		if(err < 0 )
		{
			INFO_ERROR("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
			status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();
	}
	return status;
}

E_JATIN_ERROR_CODES alsa_mute_capture_control(int in_val)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "ADCFGA Left Mute";
    int err = 0;
    E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;

	err = snd_mixer_open(&handle, 0);
    if(err < 0)
    {
        printf("Failed to open mixer %d %s",err,snd_strerror(err));
		status = E_JATIN_FAILURE;
        return status;
	}
	snd_config_update_free_global();

    if(handle)
    {
		do {
			err = snd_mixer_attach(handle, card);
			if(err < 0)
			{
				printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
    		   	status = E_JATIN_FAILURE;
				break;
			}
			err = snd_mixer_selem_register(handle, NULL, NULL);
			if(err < 0)
			{
				printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			err = snd_mixer_load(handle);
			if(err < 0)
			{
				printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			snd_mixer_selem_id_alloca(&sid);
    		if(sid == NULL)
    		{
				printf("Failed to alloc mixer element \r\n");
				status = E_JATIN_FAILURE;
				break;
    		}

			snd_mixer_selem_id_set_index(sid, 0);
			snd_mixer_selem_id_set_name(sid, selem_name);
			snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
			if(elem == NULL)
			{
				printf("Requested mixer element not found \r\n");
				status = E_JATIN_FAILURE;
				break;
			}

			err = snd_mixer_selem_set_playback_switch_all(elem,in_val);
			if(err < 0)
			{
				printf("Failed to set mute switch val   %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
		} while(0);

/*
		snd_mixer_selem_get_playback_switch(elem,0,&in_val);
                printf("get capture mute switch %d \r\n",in_val);
*/

		err = snd_mixer_close(handle);
		if(err < 0 )
		{
			status = E_JATIN_FAILURE;
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
		}
	snd_config_update_free_global();
    }
    return status;
}

E_JATIN_ERROR_CODES SetAudioCaptureAmplifierVolume(int volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "PGA Level";
    int err = 0;
    long min, max;
    E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;

    if (volume  < 0)
    {
        volume = 0;
    }
	if (volume  > 95)
    {
        volume = 95;
    }


	err = snd_mixer_open(&handle, 0);
    if(err < 0)
    {
        printf("Failed to open mixer %d %s \r\n",err,snd_strerror(err));
        return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

    if(handle)
    {
		do {
			err = snd_mixer_attach(handle, card);
			if(err < 0)
			{
				printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			err = snd_mixer_selem_register(handle, NULL, NULL);
			if(err < 0)
			{
				printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			err = snd_mixer_load(handle);
			if(err < 0)
			{
				printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			snd_mixer_selem_id_alloca(&sid);
    	    if(sid == NULL)
    	    {
				printf("Failed to alloc mixer element \r\n");
				status = E_JATIN_FAILURE;
				break;
    	    }

			snd_mixer_selem_id_set_index(sid, 0);
			snd_mixer_selem_id_set_name(sid, selem_name);
			snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
			if(elem == NULL)
			{
				printf("Requested mixer element not found \r\n");
				status = E_JATIN_FAILURE;
				break;
			}
			snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
//			printf("Max audio value %d %d \r\n",min,max);

			err  = snd_mixer_selem_set_capture_volume_all(elem, volume);
    	    if(err < 0 )
    	    {
				printf("Failed to set playback volume  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
    	    }
    	    else
    	    {
				status = E_JATIN_SUCCESS;
    	    }
		} while(0);

		err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       		status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();
	}
        return status;
}

E_JATIN_ERROR_CODES SetAudioCaptureVolume(int volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "ADC Level";
    int err = 0;
    long min, max;
    E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;

     if (volume  < 0)
     {
         volume = 0;
     }
	 if (volume  > 64)
     {
         volume = 64;
     }

	if(volume == 0 )
	{
		alsa_mute_capture_control(1);
	}
	else
	{
		alsa_mute_capture_control(0);
	}

	err = snd_mixer_open(&handle, 0);
    if(err < 0)
    {
        printf("Failed to open mixer %d %s",err,snd_strerror(err));
        return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();
    if(handle)
    {
		do {
			err = snd_mixer_attach(handle, card);
			if(err < 0)
			{
				printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			err = snd_mixer_selem_register(handle, NULL, NULL);
			if(err < 0)
			{
				printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			err = snd_mixer_load(handle);
			if(err < 0)
			{
				printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
			}
			snd_mixer_selem_id_alloca(&sid);
    	    if(sid == NULL)
    	    {
				printf("Failed to alloc mixer element \r\n");
				status = E_JATIN_FAILURE;
				break;
    	    }

			snd_mixer_selem_id_set_index(sid, 0);
			snd_mixer_selem_id_set_name(sid, selem_name);
			snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
			if(elem == NULL)
			{
				printf("Requested mixer element not found \r\n");
				status = E_JATIN_FAILURE;
				break;
			}
			snd_mixer_selem_get_capture_volume_range(elem, &min, &max);

			err  = snd_mixer_selem_set_capture_volume_all(elem, volume );
    	    if(err < 0 )
    	    {
				printf("Failed to set playback volume  %d %s \r\n",err,snd_strerror(err));
				status = E_JATIN_FAILURE;
				break;
    	    }
    	    else
    	    {
    	   	  status = E_JATIN_SUCCESS;
    	    }
		} while(0);

		err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
			status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();
	}
	return status;
}

E_JATIN_ERROR_CODES SetAudioPlaybackAmplifierVolume(int volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "HP Driver Gain";
    int err = 0;
    long min, max;
    E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;

        if (volume  < 0)
        {
            volume = 0;
        }
	  if (volume  > 35)
        {
            volume = 35;
        }


	err = snd_mixer_open(&handle, 0);
        if(err < 0)
        {
            printf("Failed to open mixer %d %s",err,snd_strerror(err));
            return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

        if(handle)
        {
			do {
		err = snd_mixer_attach(handle, card);
		if(err < 0)
		{
			printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_selem_register(handle, NULL, NULL);
		if(err < 0)
		{
			printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_load(handle);
		if(err < 0)
		{
			printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_id_alloca(&sid);
                if(sid == NULL)
                {
			printf("Failed to alloc mixer element \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
                }

		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
		if(elem == NULL)
		{
			printf("Requested mixer element not found \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
//		printf("Max audio value %d %d \r\n",min,max);

		err  = snd_mixer_selem_set_playback_volume_all(elem, volume );
                if(err < 0 )
                {
			printf("Failed to set playback volume  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
                }
                else
                {
       			  status = E_JATIN_SUCCESS;
                 }
			 } while(0);

			 err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
		}

	snd_config_update_free_global();

        }
       return status;
}

E_JATIN_ERROR_CODES SetAudioPlaybackVolume(int volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "PCM";
    int err = 0;
    long min, max;
    E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;
    int volume_level  = 0;

        if (volume  < 0)
        {
            volume = 0;
        }
	  if (volume  > 100)
        {
            volume = 100;
        }

	if(volume == 0 )
	{
		alsa_mute_playback_control(1);
	}
	else
	{
		alsa_mute_playback_control(0);
	}

    volume_level = volume_playback[volume];



	err = snd_mixer_open(&handle, 0);
	if(err < 0)
	{
		printf("Failed to open mixer %d %s",err,snd_strerror(err));
		return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

        if(handle)
        {
		do {
		err = snd_mixer_attach(handle, card);
		if(err < 0)
		{
			printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_selem_register(handle, NULL, NULL);
		if(err < 0)
		{
			printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_load(handle);
		if(err < 0)
		{
			printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_id_alloca(&sid);
                if(sid == NULL)
                {
			printf("Failed to alloc mixer element \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
                }

		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
		if(elem == NULL)
		{
			printf("Requested mixer element not found \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	//	printf("Max audio value %d %d \r\n",min,max);

		err  = snd_mixer_selem_set_playback_volume_all(elem, volume_level );
                if(err < 0 )
                {
			printf("Failed to set playback volume  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
                }
                else
                {
       			  status = E_JATIN_SUCCESS;
                 }
			 } while(0);

			 err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();

        }
    return status;
}

E_JATIN_ERROR_CODES GetAudioCaptureAmplifierVolume(int *volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "PGA Level";
	long vol_r = 0;
	int err = 0;
	long min, max;
	E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;

	err = snd_mixer_open(&handle, 0);
        if(err < 0)
        {
            printf("Failed to open mixer %d %s",err,snd_strerror(err));
            return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

        if(handle)
        {
		do {
		err = snd_mixer_attach(handle, card);
		if(err < 0)
		{
			printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_selem_register(handle, NULL, NULL);
		if(err < 0)
		{
			printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_load(handle);
		if(err < 0)
		{
			printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_id_alloca(&sid);
                if(sid == NULL)
                {
			printf("Failed to alloc mixer element \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
                }
		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
		if(elem == NULL)
		{
			printf("Requested mixer element not found \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
	 //	printf("Max audio value %d %d \r\n",min,max);
                err = snd_mixer_selem_get_capture_volume(elem,(snd_mixer_selem_channel_id_t)0,&vol_r);
		 if(err < 0 )
                {
			printf("Failed to set get PGA  capture volume  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
                }
                else
                {
       			  status = E_JATIN_SUCCESS;
                }

               *volume = vol_r ;
		   } while(0);
           //     printf(" get volume %d \r\n",*volume);
          //   snd_mixer_selem_get_capture_dB(elem,0,&vol_r);

    	err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
		}
	        }
	snd_config_update_free_global();
        return status;
}

E_JATIN_ERROR_CODES GetAudioCaptureVolume(int *volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "ADC Level";
    int err = 0;
    long min, max;
    E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;
	long vol_r = 0;

	err = snd_mixer_open(&handle, 0);
        if(err < 0)
        {
            printf("Failed to open mixer %d %s",err,snd_strerror(err));
            return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

        if(handle)
        {
		do {
		err = snd_mixer_attach(handle, card);
		if(err < 0)
		{
			printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_selem_register(handle, NULL, NULL);
		if(err < 0)
		{
			printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_load(handle);
		if(err < 0)
		{
			printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_id_alloca(&sid);
                if(sid == NULL)
                {
			printf("Failed to alloc mixer element \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
                }

		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
		if(elem == NULL)
		{
			printf("Requested mixer element not found \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
	//	printf("Max audio value %d %d \r\n",min,max);
	        err = snd_mixer_selem_get_capture_volume(elem,(snd_mixer_selem_channel_id_t)0,&vol_r);
		 if(err < 0 )
                {
			printf("Failed to get capture volume  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
                }
                else
                {
       			  status = E_JATIN_SUCCESS;
                }

               *volume = vol_r ;
		   } while(0);
              //  printf(" get volume %d \r\n",*volume);
              //  snd_mixer_selem_get_capture_dB(elem,0,&vol_r);
              //  printf(" get volumeDb %d \r\n",vol_r);
			    	err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();
        }
        return status;
}

E_JATIN_ERROR_CODES GetAudioPlaybackAmplifierVolume(int *volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "HP Driver Gain";
	int err = 0;
	long min, max;
	E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;
	long vol_r = 0;

	err = snd_mixer_open(&handle, 0);
        if(err < 0)
        {
            printf("Failed to open mixer %d %s",err,snd_strerror(err));
            return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

        if(handle)
        {
		do {
		err = snd_mixer_attach(handle, card);
		if(err < 0)
		{
			printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_selem_register(handle, NULL, NULL);
		if(err < 0)
		{
			printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_load(handle);
		if(err < 0)
		{
			printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_id_alloca(&sid);
                if(sid == NULL)
                {
			printf("Failed to alloc mixer element \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
                }

		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
		if(elem == NULL)
		{
			printf("Requested mixer element not found \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	//	printf("Max audio value %d %d \r\n",min,max);
		err = snd_mixer_selem_get_playback_volume(elem,(snd_mixer_selem_channel_id_t)0,&vol_r);
		if(err < 0 )
		{
			printf("Failed to Get PGA playback volume  %d %s \r\n",err,snd_strerror(err));
			status = E_JATIN_FAILURE;
			break;
		}
                else
                {
			status = E_JATIN_SUCCESS;
                }

               *volume = vol_r ;
               // printf(" get volume %d \r\n",*volume);
                snd_mixer_selem_get_playback_dB(elem,(snd_mixer_selem_channel_id_t)0,&vol_r);
               // printf(" get volumeDb %d \r\n",vol_r);
	   } while(0);
	   err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();

        }
       return status;
}

E_JATIN_ERROR_CODES GetAudioPlaybackVolume(int *volume)
{
	snd_mixer_t *handle = NULL;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "PCM";
	int err = 0;
	long min, max;
	E_JATIN_ERROR_CODES status = E_JATIN_SUCCESS;
	long vol_r = 0;
	int start_point = 0;
	int i = 0;

	err = snd_mixer_open(&handle, 0);
        if(err < 0)
        {
            printf("Failed to open mixer %d %s",err,snd_strerror(err));
            return E_JATIN_FAILURE;
	}
	snd_config_update_free_global();

        if(handle)
        {
		do {
		err = snd_mixer_attach(handle, card);
		if(err < 0)
		{
			printf("Failed to attach mixer %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_selem_register(handle, NULL, NULL);
		if(err < 0)
		{
			printf("Failed to register selem  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		err = snd_mixer_load(handle);
		if(err < 0)
		{
			printf("Failed to load mixer element  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_id_alloca(&sid);
                if(sid == NULL)
                {
			printf("Failed to alloc mixer element \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
                }

		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
		if(elem == NULL)
		{
			printf("Requested mixer element not found \r\n");
       			  status = E_JATIN_FAILURE;
				  break;
		}
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	//	printf("Max audio value %d %d \r\n",min,max);
		err =   snd_mixer_selem_get_playback_volume(elem,(snd_mixer_selem_channel_id_t)0,&vol_r);
		if(err < 0 )
		{
			printf("Failed to get playback volume  %d %s \r\n",err,snd_strerror(err));
			status = E_JATIN_FAILURE;
			break;
		}
		else
		{
			status = E_JATIN_SUCCESS;
		}
        if(vol_r >= 0 && vol_r <78 )
          {
              start_point = 0;
          }
        if( vol_r >=78 && vol_r < 135)
          {
               start_point = 30;
          }
       if( vol_r >= 135 &&  vol_r <= 175)
          {
                 start_point = 60;
          }

	   for(i = start_point ; i <= 100 ; i++)
	   {
		   if(volume_playback[i] == vol_r)
		   {
			   *volume = i;
		   }
	   }


                printf(" get volume %ld %d\r\n",vol_r,*volume);
			} while(0);

		err = snd_mixer_close(handle);
		if(err < 0 )
		{
			printf("Failed to close mixer handler  %d %s \r\n",err,snd_strerror(err));
       			  status = E_JATIN_FAILURE;
		}
	snd_config_update_free_global();


        }
    return status;
}
