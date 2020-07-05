/*
	Application template for Amazfit Bip BipOS
	(C) Maxim Volkov  2019 <Maxim.N.Volkov@ya.ru>
	
	Шаблон приложения для загрузчика BipOS
	
*/

/*
	Real time heart rate monitor for Amazfit Bip BipOS V2.0
	(C) Alexander Baransky (Sanya pilot) 05.07.2020 <alexander.baranskiy@yandex.ru>

	Монитор сердечного ритма в реальном времени
*/
#include <libbip.h>
#include "main.h"

//	структура меню экрана - для каждого экрана своя
struct regmenu_ screen_data = {
						55,							//	номер главного экрана, значение 0-255, для пользовательских окон лучше брать от 50 и выше
						1,							//	вспомогательный номер экрана (обычно равен 1)
						0,							//	0
						dispatch_screen,			//	указатель на функцию обработки тача, свайпов, долгого нажатия кнопки
						key_press_screen, 			//	указатель на функцию обработки нажатия кнопки
						screen_job_wrapper,			//	указатель на функцию-колбэк таймера 
						0,							//	0
						show_screen,				//	указатель на функцию отображения экрана
						0,							//	
						0		//	долгое нажатие кнопки
					};
					
int main(int param0, char** argv){	//	здесь переменная argv не определена
	show_screen((void*) param0);
}

void show_screen (void *param0){
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data;					//	указатель на данные экрана
// проверка источника запуска процедуры
if ( (param0 == *app_data_p) && get_var_menu_overlay()){ // возврат из оверлейного экрана (входящий звонок, уведомление, будильник, цель и т.д.)

	app_data = *app_data_p;					//	указатель на данные необходимо сохранить для исключения 
											//	высвобождения памяти функцией reg_menu
	*app_data_p = NULL;						//	обнуляем указатель для передачи в функцию reg_menu	

	// 	создаем новый экран, при этом указатель temp_buf_2 был равен 0 и память не была высвобождена	
	reg_menu(&screen_data, 0); 				// 	menu_overlay=0
	
	*app_data_p = app_data;						//	восстанавливаем указатель на данные после функции reg_menu
	
	set_hrm_mode(0);	//	выключаем пульсометр

	fill_screen_bg();	//	заполняем экран цветом фона (черным)
	//	здесь проводим действия при возврате из оверлейного экрана: восстанавливаем данные и т.д.
	redraw_screen();	//	перерисовываем экран вместе с графиком

	set_hrm_mode(0x20);	//	включаем пульсометр
} else { // если запуск функции произошел впервые т.е. из меню 

	// создаем экран (регистрируем в системе)
	reg_menu(&screen_data, 0);

	// выделяем необходимую память и размещаем в ней данные, (память по указателю, хранящемуся по адресу temp_buf_2 высвобождается автоматически функцией reg_menu другого экрана)
	*app_data_p = (struct app_data_ *)pvPortMalloc(sizeof(struct app_data_));
	app_data = *app_data_p;		//	указатель на данные
	
	// очистим память под данные
	_memclr(app_data, sizeof(struct app_data_));
	
	//	значение param0 содержит указатель на данные запущенного процесса структура Elf_proc_
	app_data->proc = param0;
	
	// запомним адрес указателя на функцию в которую необходимо вернуться после завершения данного экрана
	if ( param0 && app_data->proc->ret_f ) 			//	если указатель на возврат передан, то возвоащаемся на него
		app_data->ret_f = app_data->proc->elf_finish;
	else					//	если нет, то на циферблат
		app_data->ret_f = show_watchface;
	
	// здесь проводим действия которые необходимо если функция запущена впервые из меню: заполнение всех структур данных и т.д.
	struct settings_ settings;

	// читаем настройки из энергонезависимой памяти
	ElfReadSettings(app_data->proc->index_listed, &settings, 0, sizeof(settings));
	if (settings.pix_per_rec == 0){	// если кол-во пикселей на запись = 0, значит это первый запуск
		// задаем значения по-умолчанию
		settings.pix_per_rec = 10;
		settings.seconds_between_rec = 0;
		settings.minutes_for_rec = 0;
		settings.backlight = 1;
		// записываем эти значения в энергонезависимую память
		ElfWriteSettings(app_data->proc->index_listed, &settings, 0, sizeof(settings));
	}
	// заполняем структуру данных приложения
	app_data->status = 0;
	app_data->pix_per_rec = settings.pix_per_rec;
	app_data->curX = 0;			
	app_data->curY = 176;
	app_data->rec_counter = 0;
	app_data->rec_counter_per_screen = 0;
	app_data->minutes_for_rec = settings.minutes_for_rec;
	app_data->anim_counter = 0;
	app_data->seconds_between_rec = settings.seconds_between_rec;
	app_data->menu_stage = 0;
	app_data->curr_time = 0;
	app_data->backlight = settings.backlight;

	set_display_state_value(4, 1);	//	настройка подсветки: 0-принудительно выключена, 1-принудительно включена

	// здесь выполняем отрисовку интерфейса, обновление (перенос в видеопамять) экрана выполнять не нужно
	if (get_left_side_menu_active() == 1){
		show_menu_animate(first_draw, 0, ANIMATE_RIGHT);	//	в данном случае это первичная отрисовка
	} else {
		show_menu_animate(first_draw, 0, ANIMATE_LEFT);
	}
}
}

void key_press_screen(){
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	if (app_data->menu_is_on == 0){
		//	если сейчас проводится измерение, то выводим сводку по измерениям
		set_hrm_mode(0);	// выключаем пульсометр
		set_update_period(0, 0);	// выключаем таймер обновления экрана
		app_data->menu_stage = 4;
		show_menu_animate(menu, 0, ANIMATE_LEFT);	// отрисовываем меню
	} else {
		//	если запуск был из быстрого меню, при нажатии кнопки выходим на циферблат
		if ( get_left_side_menu_active(	) ){		
		    app_data->proc->ret_f = show_watchface;
		}
		// вызываем функцию возврата (обычно это меню запуска), в качестве параметра указываем адрес функции нашего приложения
		show_menu_animate(app_data->ret_f, (unsigned int)show_screen, ANIMATE_RIGHT);
	}
}

int dispatch_screen (void *param){
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

// в случае отрисовки интерфейса, обновление (перенос в видеопамять) экрана выполнять нужно

struct gesture_ *gest = param;
int result = 0;

switch (gest->gesture){
	case GESTURE_CLICK: {		
			if (app_data->menu_is_on == 1){
				//	увеличение или уменьшение значений на экранах настройки
				if (app_data->menu_stage == 1){
					if ((gest->touch_pos_x >= 20 && gest->touch_pos_x <= 70) && (gest->touch_pos_y >= 55 && gest->touch_pos_y <= 105)){
						app_data->pix_per_rec = app_data->pix_per_rec - 5;
						if (app_data->pix_per_rec < 5){
							app_data->pix_per_rec = 5;
						}
						set_fg_color(COLOR_WHITE);
						char text[10];
						_sprintf(text, "%d", app_data->pix_per_rec);
						draw_filled_rect_bg(FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 109, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
						show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 2);
						repaint_screen_lines(FIRST_MENU_BIG_DIGITS_COORD_Y, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
					}
					if ((gest->touch_pos_x >= 100 && gest->touch_pos_x <= 150) && (gest->touch_pos_y >= 55 && gest->touch_pos_y <= 105)){
						app_data->pix_per_rec = app_data->pix_per_rec + 5;
						if (app_data->pix_per_rec > 30){
							app_data->pix_per_rec = 30;
						}
						set_fg_color(COLOR_WHITE);
						char text[10];
						_sprintf(text, "%d", app_data->pix_per_rec);
						draw_filled_rect_bg(FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 109, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
						show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 2);
						repaint_screen_lines(FIRST_MENU_BIG_DIGITS_COORD_Y, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
					}
				} else if (app_data->menu_stage == 2){
					if ((gest->touch_pos_x >= 20 && gest->touch_pos_x <= 70) && (gest->touch_pos_y >= 55 && gest->touch_pos_y <= 105)){
						app_data->seconds_between_rec = app_data->seconds_between_rec - 1;
						if (app_data->seconds_between_rec < 0){
							app_data->seconds_between_rec = 60;
						}
						set_fg_color(COLOR_WHITE);
						char text[10];
						_sprintf(text, "%d", app_data->seconds_between_rec);
						draw_filled_rect_bg(FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 109, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
						show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 2);
						repaint_screen_lines(FIRST_MENU_BIG_DIGITS_COORD_Y, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
					}
					if ((gest->touch_pos_x >= 100 && gest->touch_pos_x <= 150) && (gest->touch_pos_y >= 55 && gest->touch_pos_y <= 105)){
						app_data->seconds_between_rec = app_data->seconds_between_rec + 1;
						if (app_data->seconds_between_rec > 60){
							app_data->seconds_between_rec = 0;
						}
						set_fg_color(COLOR_WHITE);
						char text[10];
						_sprintf(text, "%d", app_data->seconds_between_rec);
						draw_filled_rect_bg(FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 109, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
						show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 2);
						repaint_screen_lines(FIRST_MENU_BIG_DIGITS_COORD_Y, FIRST_MENU_BIG_DIGITS_COORD_Y + 50);
					}
				} else if (app_data->menu_stage == 3){
					if ((gest->touch_pos_x >= 20 && gest->touch_pos_x <= 70) && (gest->touch_pos_y >= 55 + 20 && gest->touch_pos_y <= 105 + 20)){
						app_data->minutes_for_rec = app_data->minutes_for_rec - 1;
						if (app_data->minutes_for_rec < 0){
							app_data->minutes_for_rec = 60;
						}
						set_fg_color(COLOR_WHITE);
						char text[10];
						_sprintf(text, "%d", app_data->minutes_for_rec);
						draw_filled_rect_bg(FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y + 20, 109, FIRST_MENU_BIG_DIGITS_COORD_Y + 50 + 20);
						show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y + 20, 2);
						repaint_screen_lines(FIRST_MENU_BIG_DIGITS_COORD_Y + 20, FIRST_MENU_BIG_DIGITS_COORD_Y + 50 + 20);
					}
					if ((gest->touch_pos_x >= 100 && gest->touch_pos_x <= 150) && (gest->touch_pos_y >= 55 && gest->touch_pos_y <= 105)){
						app_data->minutes_for_rec = app_data->minutes_for_rec + 1;
						if (app_data->minutes_for_rec > 60){
							app_data->minutes_for_rec = 0;
						}
						set_fg_color(COLOR_WHITE);
						char text[10];
						_sprintf(text, "%d", app_data->minutes_for_rec);
						draw_filled_rect_bg(FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y + 20, 109, FIRST_MENU_BIG_DIGITS_COORD_Y + 50 + 20);
						show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y + 20, 2);
						repaint_screen_lines(FIRST_MENU_BIG_DIGITS_COORD_Y + 20, FIRST_MENU_BIG_DIGITS_COORD_Y + 50 + 20);
					}
				}
				//	обработка нажатия на нижнюю часть экрана в экранах настройки
				if (gest->touch_pos_y >= 135 && gest->touch_pos_y <= 176){
					if (app_data->menu_stage == 0){
						if (gest->touch_pos_x >= 0 && gest->touch_pos_x < 88){
							app_data->backlight = 0;
							set_backlight_state(0);
							update_settings();
							app_data->menu_stage = app_data->menu_stage + 1;
							show_menu_animate(menu, 0, ANIMATE_LEFT);
						} else if (gest->touch_pos_x > 88 && gest->touch_pos_x <= 176){
							app_data->backlight = 1;
							set_backlight_state(1);
							update_settings();
							app_data->menu_stage = app_data->menu_stage + 1;
							show_menu_animate(menu, 0, ANIMATE_LEFT);
						}
					} else if (app_data->menu_stage == 3){
						app_data->menu_is_on = 0;
						app_data->menu_stage = 0;
						update_settings();	//	обновление настроек в энергонезависимой памяти
						show_menu_animate(first_draw, 0, ANIMATE_UP);	// после настройки, начинаем заново отрисовывать график
					} else if (app_data->menu_stage == 4){
						//	если запуск был из быстрого меню, при нажатии кнопки выходим на циферблат
						if ( get_left_side_menu_active() ) 		
						    app_data->proc->ret_f = show_watchface;
							
						// вызываем функцию возврата (обычно это меню запуска), в качестве параметра указываем адрес функции нашего приложения
						show_menu_animate(app_data->ret_f, (unsigned int)show_screen, ANIMATE_RIGHT);
					} else {
						app_data->menu_stage = app_data->menu_stage + 1;	// переключаем экран меню
						update_settings();	//	обновление настроек в энергонезависимой памяти
						show_menu_animate(menu, 0, ANIMATE_LEFT);	// заново отрисовываем меню
					}
				}
			}
			break;
		};
		case GESTURE_SWIPE_RIGHT: 	//	свайп направо
		case GESTURE_SWIPE_LEFT: {	// справа налево
			if (app_data->menu_is_on == 1){
				if ( get_left_side_menu_active()){
					// в случае запуска через быстрое меню в proc->ret_f содержится dispatch_left_side_menu
					// и после отработки elf_finish (на который указывает app_data->ret_f, произойдет запуск dispatch_left_side_menu c
					// параметром структуры события тачскрина, содержащемся в app_data->proc->ret_param0
					
					// запускаем dispatch_left_side_menu с параметром param в результате произойдет запуск соответствующего бокового экрана
					// при этом произойдет выгрузка данных текущего приложения и его деактивация.
					void* show_f = get_ptr_show_menu_func();
					dispatch_left_side_menu(param);
										
					if ( get_ptr_show_menu_func() == show_f ){
						// если dispatch_left_side_menu вернет GESTURE_SWIPE_RIGHT то левее экрана нет, листать некуда
						// просто игнорируем этот жест
						return 0;
					}
					
					//	если dispatch_left_side_menu отработал, то завершаем наше приложение, т.к. данные экрана уже выгрузились
					// на этом этапе уже выполняется новый экран (тот куда свайпнули)
					Elf_proc_* proc = get_proc_by_addr(main);
					proc->ret_f = NULL;
					
					elf_finish(main);	//	выгрузить Elf из памяти
					return 0;
				} else { 			//	если запуск не из быстрого меню, обрабатываем свайпы по отдельности
					switch (gest->gesture){
						case GESTURE_SWIPE_RIGHT: {	//	свайп направо
							//	отключить измерение
							set_hrm_mode(0);
			
							return show_menu_animate(app_data->ret_f, (unsigned int)main, (gest->gesture == GESTURE_SWIPE_RIGHT) ? ANIMATE_RIGHT : ANIMATE_LEFT);
							break;
						}
						case GESTURE_SWIPE_LEFT: {	// справа налево
							//	действие при запуске из меню и дальнейший свайп влево
							
							
							break;
						}
					} /// switch (gest->gesture)
				}
			}

			break;
		};	//	case GESTURE_SWIPE_LEFT:
		
			
		case GESTURE_SWIPE_UP: {	// свайп вверх
			
			break;
		};
		case GESTURE_SWIPE_DOWN: {	// свайп вниз, переходим в меню настройки
			set_hrm_mode(0);	// выключаем пульсометр
			set_update_period(0, 0);	// выключаем таймер обновления экрана
			show_menu_animate(menu, 0, ANIMATE_DOWN);	// отрисовываем меню
			break;
		};		
		default:{	// что-то пошло не так...
			
			break;
		};		
		
	}	//	switch (gest->gesture)
	
	return result;
};

void menu(){	//	 функция рисования экранов настройки
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	app_data->menu_is_on = 1;

	set_bg_color(COLOR_BLACK);	// устанавливаем цвет фона
	set_fg_color(COLOR_WHITE);	// устанавливаем цвет текста
	fill_screen_bg();	//	заполняем экран этим цветом

	if (app_data->menu_stage == 0){
		text_out_center("Включить подсветку?", 88, 65);	// выводим текст

		// рисуем кнопки "Да" и "Нет"
		set_fg_color(COLOR_RED);
		draw_filled_rect(0, 136, 88, 176);
		show_res_by_id(ICON_CANCEL_RED, 37, 149);

		set_fg_color(COLOR_GREEN);
		draw_filled_rect(88, 136, 176, 176);
		show_res_by_id(ICON_OK_GREEN, 125, 149);
	} else if (app_data->menu_stage == 1){
		// переделываем int в char[] при помощи sprintf
		char text[10];
		_sprintf(text, "%d", app_data->pix_per_rec);
		// отрисовываем большие цифры
		show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 2);
		
		// отрисовываем кнопки "+" и "-" картинками из ресурсов elf'а
		show_elf_res_by_id(app_data->proc->index_listed, 1, 30, 65);
		show_elf_res_by_id(app_data->proc->index_listed, 0, 110, 65);
		
		// выводим текст
		text_out_center("Кол-во пикселей", 88, 5);
		text_out_center("на одну запись", 88, get_text_height() + 6);

		// отрисовываем кнопку перехода на следующую страницу
		set_fg_color(COLOR_AQUA);
		draw_filled_rect(0, 136, 176, 176);
		show_res_by_id(317, 147, 147);
	} else if (app_data->menu_stage == 2){
		char text[10];
		_sprintf(text, "%d", app_data->seconds_between_rec);
		show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y, 2);
	
		show_elf_res_by_id(app_data->proc->index_listed, 1, 30, 65);
		show_elf_res_by_id(app_data->proc->index_listed, 0, 110, 65);
	
		text_out_center("Задержка между", 88, 5);
		text_out_center("измерениями (сек.)", 88, get_text_height() + 6);

		set_fg_color(COLOR_AQUA);
		draw_filled_rect(0, 136, 176, 176);
		show_res_by_id(317, 147, 147);
	} else if (app_data->menu_stage == 3){
		char text[10];
		_sprintf(text, "%d", app_data->minutes_for_rec);
		show_big_digit(3, text, FIRST_MENU_BIG_DIGITS_COORD_X, FIRST_MENU_BIG_DIGITS_COORD_Y + 20, 2);
	
		show_elf_res_by_id(app_data->proc->index_listed, 1, 30, 65 + 20);
		show_elf_res_by_id(app_data->proc->index_listed, 0, 110, 65 + 20);
	
		text_out_center("Общее время", 88, 5);
		text_out_center("измерения (мин.)", 88, get_text_height() + 6);
		text_out_center("0-нет ограничения", 88, 2 * get_text_height() + 6);

		set_fg_color(COLOR_AQUA);
		draw_filled_rect(0, 136, 176, 176);
		show_res_by_id(317, 147, 147);
	} else if (app_data->menu_stage == 4){
		vibrate(2, 300, 300);	// уведомляем пользователя об окончании измерения вибрацией

		text_out_center("Сводка по", 88, 5);
		text_out_center("измерениям", 88, get_text_height() + 5);
		text_out_center("MIN", (text_width("MIN") / 2) + 15, 2 * get_text_height() + 10);
		text_out_center("MAX", (176 - (text_width("MAX") / 2)) - 15, 2 * get_text_height() + 10);
		text_out_center("AVG", 88, 2 * get_text_height() + 10);

		char min[10];
		_sprintf(min, "%d", find_min());
		show_big_digit(3, min, 2, 3 * get_text_height() + 15, 2);

		char max[10];
		_sprintf(max, "%d", find_max());
		show_big_digit(3, max, 120, 3 * get_text_height() + 15, 2);

		char avg[10];
		_sprintf(avg, "%d", find_avg());
		show_big_digit(3, avg, 63, 3 * get_text_height() + 15, 2);
		
		set_fg_color(COLOR_AQUA);
		draw_filled_rect(0, 136, 176, 176);
		show_res_by_id(317, 147, 147);
	}
}

void first_draw(){	//	функция начальной отрисовки элементов экрана измерения
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	// заполняем структуру данных приложения
	app_data->curX = 0;			
	app_data->curY = 176;
	app_data->rec_counter = 0;
	app_data->rec_counter_per_screen = 0;
	app_data->anim_counter = 0;
	app_data->menu_stage = 0;
	app_data->curr_time = 0;

	set_backlight_state(app_data->backlight);

	set_bg_color(COLOR_BLACK);	//	устанавливаем цвет фона
	fill_screen_bg();	//	заполняем экран этим цветом
	
	set_fg_color(COLOR_WHITE);	//	устанавливаем цвет текста и линий
	show_big_digit(3, "0", BIG_DIGITS_COORD_X, BIG_DIGITS_COORD_Y, 2); 	//	отображение цифр большим шрифтом
	/*
	text_out("min:", STATS_COORD_X, STATS_COORD_Y);
	text_out("max:", STATS_COORD_X, STATS_COORD_Y + get_text_height());
	*/
	draw_scale();	//	рисуем шкалу
	
	show_res_by_id(ICON1_RES_ID, ICON_COORD_X, ICON_COORD_Y);	//	рисуем сердце

	set_hrm_mode(0x20);	// включаем пульсометр
	set_update_period(1, 200);	// включаем таймер обновления экрана
	return;	
}

void redraw_screen(){	//	функция перерисовки графика после оверлейных экранов
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	//	возвращаем начальные координаты графика
	app_data->curX = 0;			
	app_data->curY = 176;

	// вычисляем индекс первой записи на текущем экране
	int i;
	if (app_data->rec_counter > app_data->rec_counter_per_screen){
		i = app_data->rec_counter - app_data->rec_counter_per_screen;
	} else {
		i = 0;	// если первый экран, то индекс 0
	}
	app_data->rec_counter_per_screen = 0;
	//	в цикле заново отрисовываем график, т. к. он стирается после оверлейного экрана.
	for (; i < app_data->rec_counter; i++){
		int heartrate = app_data->records[i];
		//	т. к. координата y = 0 сверху экрана, а не снизу, пересчитываем необходимые координаты
		int Y = 176 - heartrate + SCALE_OFFSET;
		//	в зависимости от того, повысился пульс или понизился, правильно отрисовываем вертикальную линию,
		//	т. к. она рисуется только в одну сторону (от меньшей координаты к большей)
		if (Y < app_data->curY){
			draw_vertical_line(app_data->curX, Y, app_data->curY);	
		} else if (Y > app_data->curY){
			draw_vertical_line(app_data->curX, app_data->curY, Y);
		}
		//	Здесь мы рисуем горизонтальную линию, но пришлось применить костыль со счетчиком записей, т. к. эта линия рисовалась только у первой записи.
		//	Оригинальный вариант закоментирован ниже. Если вы знаете, как решить эту проблему, прошу связаться со мной в личке форума MyAmazfit.ru или 4PDA. Ник написан вначале файла.
		//draw_horizontal_line(Y, app_data->curX, app_data->pix_per_rec);
		draw_horizontal_line(Y, app_data->pix_per_rec * app_data->rec_counter_per_screen, app_data->pix_per_rec * (app_data->rec_counter_per_screen + 1));
		//	заполняем структуру данными
		app_data->curX = app_data->curX + app_data->pix_per_rec;
		app_data->curY = Y;
		app_data->rec_counter_per_screen = app_data->rec_counter_per_screen + 1;
		repaint_screen_lines(0, 176);	//	обновление экрана
	}
	draw_scale();	//	рисуем шкалу
	char text[10];
	_sprintf(text, "%d", app_data->records[app_data->rec_counter]);
	show_big_digit(3, text, BIG_DIGITS_COORD_X, BIG_DIGITS_COORD_Y, 2);	//	отображение цифр большим шрифтом
	//	выводим на экран минимальное и максимальное значения пульса
	char min[10];
	_sprintf(min, "%s%d", "min:", find_min());
	text_out(min, STATS_COORD_X, STATS_COORD_Y);
	
	char max[10];
	_sprintf(max, "%s%d", "max:", find_max());
	text_out(max, STATS_COORD_X, STATS_COORD_Y + get_text_height() + STATS_COORD_Y);

	switch (app_data->anim_counter){	//	анимация сердца
	case 0:
		show_res_by_id(ICON1_RES_ID, ICON_COORD_X, ICON_COORD_Y);
		app_data->anim_counter = 1;
		break;

	case 1:
		show_res_by_id(ICON2_RES_ID, ICON_COORD_X, ICON_COORD_Y);
		app_data->anim_counter = 0;
		break;
	}
}

void draw_scale(){	//	функция рисования шкалы
	load_font();	//	подгрузка шрифтов
	int i = 10;
	char num[3];
	//	в цикле отрисовываем шкалу при помощи шрифта, для большего диапазона графика она смещается вниз оффсетом
	while (i <= 140){
		int Y = 176 - i;
		_sprintf(num, "%d", i + SCALE_OFFSET);
		text_out(num, 176 - text_width(num), Y - (get_text_height(num) / 2));
		i = i + 20;
	}
}

int find_min(){	//	функция поиска минимального значения в массиве
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	//	в цикле перебираем массив и выбираем минимальное значение
	int min = app_data->records[0];
	for (int i = 0; i < app_data->rec_counter; i++){
		if (min > app_data->records[i]){
			min = app_data->records[i];
		}
	}
	return min;	//	возвращаем минимальное значение
}

int find_max(){	//	функция поиска минимального значения в массиве
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	//	в цикле перебираем массив и выбираем максимальное значение
	int max = app_data->records[0];
	for (int i = 0; i < app_data->rec_counter; i++){
		if (max < app_data->records[i]){
			max = app_data->records[i];
		}
	}
	return max;	//	возвращаем максимальное значение
}

int find_avg(){	//	функция поиска среднего арифметического значения в массиве
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	//	в цикле перебираем массив и вычисляем сумму всех его элементов
	int sum = 0;
	for (int i = 0; i < app_data->rec_counter; i++){
		sum += app_data->records[i];
	}
	return sum / app_data->rec_counter;	//	возвращаем среднее арифметическое значение
}

void update_settings(){
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

	struct settings_ settings;

	// записываем данные из структуры settings в app_data
	settings.pix_per_rec = app_data->pix_per_rec;
	settings.minutes_for_rec = app_data->minutes_for_rec;
	settings.seconds_between_rec = app_data->seconds_between_rec;
	settings.backlight = app_data->backlight;

	// записываем настройки в энергонезависимую память
	ElfWriteSettings(app_data->proc->index_listed, &settings, 0, sizeof(settings));
}

int screen_job_wrapper(){	//	обертка для screen_job для исправления бага и преждевременным рисованием графика
	struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
	struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана
	
	if (app_data->menu_is_on == 0){
		screen_job();
	}
	return 0;
}

int screen_job(){
// при необходимости можно использовать данные экрана в этой функции
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

if (app_data->minutes_for_rec != 0){	// если установлено ограничение по времени, считаем время в миллисекундах
	app_data->curr_time = app_data->curr_time + app_data->curr_update_period;
}

void* hrm_data = get_hrm_struct();	//	получаем структуру данных пульсометра
int heartrate;

if (get_fw_version() == NOT_LATIN_1_1_2_05){	//	определение версии прошивки для работы с пульсометром
	app_data->status = ((hrm_data_struct_legacy*)hrm_data)->ret_code;
	heartrate = ((hrm_data_struct_legacy*)hrm_data)->heart_rate;
} else {
	app_data->status = ((hrm_data_struct*)hrm_data)->ret_code;
	heartrate = ((hrm_data_struct*)hrm_data)->heart_rate;
}

set_hrm_mode(0x20);	// включаем пульсометр

switch (app_data->status){	//	обрабатываем состояния датчика
	default:
	case 5:{		//	замер не завершен
		set_update_period(1, 200);	//	повторно заводим таймер
		app_data->curr_update_period = 200;
		break;
	}

	case 0:{		//	замер завершен успешно
		//	проверяем, дошла ли линия графика до шкалы, если да, то начинаем рисовать график сначала экрана
		if ((130 - app_data->curX) < app_data->pix_per_rec){
			app_data->curX = 0;
			app_data->rec_counter_per_screen = 0;
			fill_screen_bg();
			draw_scale();
		}
		set_fg_color(COLOR_WHITE);
		char text[10];
		_sprintf(text, "%d", heartrate);
		//	закрашиваем предыдущие цифры, чтобы избежать остаточных пикселей
		draw_filled_rect_bg(ICON_COORD_X + 32, 0, STATS_COORD_X, 50);
		show_big_digit(3, text, BIG_DIGITS_COORD_X, BIG_DIGITS_COORD_Y, 2);	//	отображение цифр большим шрифтом
		//	т. к. координата y = 0 сверху экрана, а не снизу, пересчитываем необходимые координаты
		int Y = (176 - heartrate) + SCALE_OFFSET;
		//	в зависимости от того, повысился пульс или понизился, правильно отрисовываем вертикальную линию
		if (Y < app_data->curY){
			draw_vertical_line(app_data->curX, Y, app_data->curY);	
		} else if (Y > app_data->curY){
			draw_vertical_line(app_data->curX, app_data->curY, Y);
		}
		//	Здесь мы рисуем горизонтальную линию, но пришлось применить костыль со счетчиком записей, т. к. эта линия рисовалась только у первой записи.
		//	Оригинальный вариант закоментирован ниже. Если вы знаете, как решить эту проблему, прошу связаться со мной в личке форума MyAmazfit.ru или 4PDA. Ник написан вначале файла.
		//draw_horizontal_line(Y, app_data->curX, app_data->pix_per_rec);
		draw_horizontal_line(Y, app_data->pix_per_rec * app_data->rec_counter_per_screen, app_data->pix_per_rec * (app_data->rec_counter_per_screen + 1));
		//	заполняем структуру данными
		app_data->records[app_data->rec_counter] = heartrate;
		app_data->rec_counter = app_data->rec_counter + 1;
		app_data->rec_counter_per_screen = app_data->rec_counter_per_screen + 1;
		app_data->curX = app_data->curX + app_data->pix_per_rec;
		app_data->curY = Y;

		set_hrm_mode(0); 	//	отключаем пульсометр для экономии батареи (не работает с задержкой 0 - 10 секунд)
		//	выводим на экран минимальное и максимальное значения пульса
		draw_filled_rect_bg(STATS_COORD_X, STATS_COORD_Y, STATS_COORD_X + text_width("max:160"), 2 * STATS_COORD_Y + get_text_height());
		char max[10];
		_sprintf(max, "%s%d", "max:", find_max());
		text_out(max, STATS_COORD_X, STATS_COORD_Y);
		
		char min[10];
		_sprintf(min, "%s%d", "min:", find_min());
		text_out(min, STATS_COORD_X, 2 * STATS_COORD_Y + get_text_height());

		int time;
		if (app_data->seconds_between_rec == 0){
			time = 200;
		} else {
			time = app_data->seconds_between_rec * 1000;
		}
		set_update_period(1, time);	//	заново заводим таймер
		app_data->curr_update_period = time;
		break;
	} 

	case 2:{		//	замер завершен неуспешно
		vibrate(2, 100, 100);	//	предупреждаем пользователя вибрацией
		set_hrm_mode(0x20);	//	заново включаем пульсометр
		set_update_period(1, 200);	//	заново заводим таймер
		app_data->curr_update_period = 200;
		break;
	}
	
}

switch (app_data->anim_counter){	//	анимация сердца
	case 0:
		show_res_by_id(ICON1_RES_ID, ICON_COORD_X, ICON_COORD_Y);	// отрисовываем сердце из ресурсов прошивки
		app_data->anim_counter = 1;
		break;

	case 1:
		show_res_by_id(ICON2_RES_ID, ICON_COORD_X, ICON_COORD_Y);
		app_data->anim_counter = 0;
		break;
}

repaint_screen_lines(0,176);	//	обновляем экран

// отображение сводки при истечении времени измерения
if (app_data->curr_time >= app_data->minutes_for_rec * 60000 && app_data->minutes_for_rec != 0){
	set_hrm_mode(0);
	set_update_period(0, 0);
	app_data->menu_stage = 4;
	show_menu_animate(menu, 0, ANIMATE_LEFT);
}

// защита от переполнения массива
if (app_data->rec_counter == 1000){
	set_hrm_mode(0);
	set_update_period(0, 0);
	app_data->menu_stage = 4;
	show_menu_animate(menu, 0, ANIMATE_LEFT);
}
return 0;
}
