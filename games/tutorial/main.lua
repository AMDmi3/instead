game.codepage="UTF-8"
game.dsc = [[Команды:^
    look(или просто ввод), act <на что> (или просто на что), use <что> [на что], go <куда>,^
    back, inv, way, obj, quit, save <fname>, load <fname>. Работает автодополнение по табуляции.^^]];

main = room {
	nam = 'туториал',
	act = function()
		return goto('r1');
	end,
	dsc = [[Добро пожаловать в режим обучения instead (sdl версия). 
Каждая сцена игры представляет собой описание статической и динамической части.^^ 

Динамическая часть сцены состоит из объектов, персонажей и т.д. С динамической частью
игрок может взаимодействовать с помощью мыши, с помощью нажатий на подсвеченные ссылки.^^

В данной сцене единственным объектом является объект "Дальше" -- который вы видите внизу текста. 
Итак, для продолжения обучения вы можете нажать на "Дальше".
]],
	obj = { 
		vobj(1, 'Дальше', '{Дальше}'),
	},
};
apple = obj {
	nam = 'яблоко',
	dsc = 'На полу лежит {яблоко}.',
	tak = 'Вы взяли яблоко.',
	inv = function(s)
		s._seen = true;
		if s._knife then
			inv():del('apple');
			return 'Яблоко почищено! Вы его съели.';
		end
		return 'Выглядит аппетитно!';
	end,
	used = function(s, w)
		if w == 'knife' and not s._knife and here() == r4 then
			s._knife = true;
			return 'Вы чистите яблоко.';
		end
	end,
};
knife = obj {
	nam = 'нож',
	dsc = 'На полу лежит {нож}.',
	tak = 'Вы взяли нож.',
	inv = function(s)
		s._seen = true;
		return 'Острый нож.';
	end,
};

r1 = room {
	enter = function()
		lifeon('r1');
	end,
	exit = function()
		lifeoff('r1');
	end,
	life = function()
		if have('apple') and have('knife') then
			return goto('r2');
		end
	end,
	nam = 'урок 1',
	dsc = [[Продолжим урок. Сейчас вы находитесь в комнате. Возьмите оба предмета, которые вы видите в этой комнате.
Напомним, что для этого вы просто используете мышь.]],
	obj = { 'apple', 'knife' },
};

r2 = room {
	nam = 'урок 2',
	enter = function()
		lifeon('r2');
	end,
	exit = function()
		lifeoff('r2');
	end,
	life = function()
		if apple._seen and knife._seen then
			return goto('r3');
		end
	end,
	dsc = [[Отлично!! Теперь у вас появились предметы, которые вы можете использовать или изучать.
Для этого используется инвентарь. Вы можете смотреть на предметы инвентаря с помощью двойного щелчка мыши.^^
Итак, посмотрите на ножик. Затем, повторите эту операцию с яблоком.]],
};

tabl = obj {
	nam = 'стол',
	dsc = 'На этом уроке вы видите деревянный {стол}.',
	act = function(s)
		if s._knife then
			s._seen = true;
			return 'На столе стоит ваш автограф.';
		end
		return 'Обычный стол, из дуба.';
	end,
	used = function(s, w)
		if w == 'knife' and not s._knife then
			s._knife = true;
			return 'Вы вырезаете что-то ножиком на столе.';
		end
	end,
	obj = { 'vasa' },
};

vasa = obj {
	nam = 'ваза',
	dsc = 'На столе стоит {ваза}.';
	act = 'Ваза как ваза.';
}

r3 = room {
	nam = 'урок 3',
	enter = function()
		lifeon('r3');
	end,
	exit = function()
		lifeoff('r3');
	end,
	life = function()
		if tabl._seen then
			return goto('r4');
		end
	end,
	dsc = [[Хорошо! Вы можете действовать
предметами инвентаря на другие предметы сцены или инвентаря. Попробуйте использовать нож на стол. 
Для этого нажмите мышью на нож, а за затем на стол. Затем посмотрите на стол.]],
	obj = { 'tabl' },
};

r4 = room {
	nam = 'урок 4',
	enter = function()
		apple._knife = false;
		lifeon('r4');
	end,
	exit = function()
		lifeoff('r4');
	end,
	life = function()
		if not have('apple') then
			return goto('r5');
		end
	end,
	dsc = [[Ладно, а теперь -- почистите яблоко. Затем съешьте яблоко.]],
};

r5 = room {
	nam = 'урок 5',
	dsc = [[Хорошо. Изучаем переходы. Идите на урок 6. Для этого нажмите мышью на соответствующую ссылку.]],
	exit = function(s, t)
		if t ~= 'r6' then
			return 'Я просил идти на урок 6...', false;
		end
	end,
	way = { 'r1', 'r2', 'r3', 'r4', 'r6'},
};

r6 = room {
	nam = 'урок 6',
	dsc = [[Идите на последний урок.]],
	exit = function(s, t)
		if t ~= 'theend' then
			return 'Я просил идти на последний урок...', false;
		end
	end,
	way = { 'r1', 'r2', 'r3', 'r4', 'r6', 'theend'},
};

theend = room {
	nam = 'последний урок',
	dsc = [[Вы можете сохранять игру, выбрать другую игру или выполнять иные действия с помощью меню.
Для вызова меню нажмите клавишу esc или нажмите мышью на символ меню (справа снизу).
Теперь вы готовы к игре. Удачи!!!]],
};

