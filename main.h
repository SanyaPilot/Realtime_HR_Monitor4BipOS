/*
	Application template for Amazfit Bip BipOS
	(C) Maxim Volkov  2019 <Maxim.N.Volkov@ya.ru>
	
	Шаблон приложения, заголовочный файл

*/

/*
	Real time heart rate monitor for Amazfit Bip BipOS V1
	(C) Alexander Baransky (Sanya pilot) 24.06.2020 <alexander.baranskiy@yandex.ru>

	Монитор сердечного ритма в реальном времени, заголовочный файл
*/

#ifndef __APP_TEMPLATE_H__
#define __APP_TEMPLATE_H__

#define	FIRST_MENU_BIG_DIGITS_COORD_X	70
#define	FIRST_MENU_BIG_DIGITS_COORD_Y	60

#define ICON1_RES_ID	98
#define ICON2_RES_ID	99
#define ICON_COORD_X	0
#define ICON_COORD_Y	8

#define	BIG_DIGITS_COORD_X	33
#define	BIG_DIGITS_COORD_Y	5

#define	STATS_COORD_X	90
#define	STATS_COORD_Y	5

#define	SCALE_OFFSET	30

// структура данных для нашего экрана
struct app_data_ {
			void* 	ret_f;					//	адрес функции возврата
			int 	status,					//	
					menu_is_on,				//	открыто ли первичное меню
					menu_stage,				//	стадия первичного меню
					pix_per_rec,			//	количество пикселей на одну запись
					curX,					//	текущий X
					curY,					// 	текущий Y
					rec_counter,			//	счетчик записей
					rec_counter_per_screen,	//	счетчик записей для рисования горизонтальных линий
					anim_counter,			//	счетчик для анимации
					seconds_between_rec,	//	задержка между измерениями в миллисекундах
					minutes_for_rec,		//	ограничение по времени работы в минутах
					curr_update_period,		//	текущее время обновления экрана
					records[100];			//	массив записей
			long	curr_time;				//	текущее время
 			Elf_proc_* proc;				//	указатель на данные процесса
};

void 	show_screen (void *return_screen);
void 	key_press_screen();
int 	dispatch_screen (void *param);
int		screen_job_wrapper();
int 	screen_job();
void	menu();
void	first_draw();
void	redraw_screen();
void	draw_scale();
int		find_min();
int		find_max();
int		find_avg();
#endif