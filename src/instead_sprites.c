/*
 * Copyright 2009-2016 Peter Kosyh <p.kosyh at gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "externals.h"
#include "internals.h"

static LIST_HEAD(sprites);

static LIST_HEAD(fonts);

typedef struct {
	struct list_node list;
	char	*name;
	fnt_t	fnt;
} _fnt_t;

typedef struct {
	struct list_node list;
	char	*name;
	img_t	img;
} _spr_t;

static void sprites_free(void)
{
/*	fprintf(stderr, "sprites free \n"); */
	while (!list_empty(&sprites)) {
		_spr_t *sp = list_top(&sprites, _spr_t, list);
		free(sp->name);
		cache_forget(gfx_image_cache(), sp->img);
		list_del(&sp->list);
		free(sp);
	}
	while (!list_empty(&fonts)) {
		_fnt_t *fn = list_top(&fonts, _fnt_t, list);
		fnt_free(fn->fnt);
		free(fn->name);
		list_del(&fn->list);
		free(fn);
	}
	game_pict_modify(NULL);
	cache_shrink(gfx_image_cache());
}

static _spr_t *sprite_lookup(const char *name)
{
	_spr_t *pos = NULL;
	_spr_t *sp;
	list_for_each(&sprites, pos, list) {
		sp = (_spr_t*)pos;
		if (!strcmp(name, sp->name)) {
			list_del(&sp->list);
			list_add(&sprites, &sp->list); /* move it on head */
			return sp;
		}
	}
	return NULL;
}

static _fnt_t *font_lookup(const char *name)
{
	_fnt_t *pos = NULL;
	_fnt_t *fn;
	list_for_each(&fonts, pos, list) {
		fn = (_fnt_t*)pos;
		if (!strcmp(name, fn->name)) {
			list_del(&fn->list);
			list_add(&fonts, &fn->list); /* move it on head */
			return fn;
		}
	}
	return NULL;
}

static _spr_t *sprite_new(const char *name, img_t img)
{
	_spr_t *sp;
	sp = malloc(sizeof(_spr_t));
	if (!sp)
		return NULL;
/*	INIT_LIST_HEAD(&sp->list); */
	sp->name = strdup(name);
	if (!sp->name) {
		free(sp);
		return NULL;
	}
	sp->img = img;
	if (cache_add(gfx_image_cache(), name, img)) {
		free(sp->name);
		free(sp);
		return NULL;
	}
/*	fprintf(stderr, "added: %s\n", name); */
	list_add(&sprites, &sp->list);
	return sp;
}

static _fnt_t *font_new(const char *name, fnt_t fnt)
{
	_fnt_t *fn;
	fn = malloc(sizeof(_fnt_t));
	if (!fn)
		return NULL;
/*	INIT_LIST_HEAD(&fn->list); */
	fn->name = strdup(name);
	if (!fn->name) {
		free(fn);
		return NULL;
	}
	fn->fnt = fnt;
	list_add(&fonts, &fn->list);
	return fn;
}

static void sprite_name(const char *name, char *sname, int size)
{
	unsigned long h = 0;
	if (!sname || !size)
		return;
	h = hash_string(name);
	do { /* new uniq name */
		snprintf(sname, size, "spr:%lx", h);
		h ++;
	} while (sprite_lookup(sname) || cache_lookup(gfx_image_cache(), sname));
	sname[size - 1] = 0;
}

static void font_name(const char *name, char *sname, int size)
{
	unsigned long h = 0;
	if (!sname || !size)
		return;
	h = hash_string(name);
	do { /* new uniq name */
		snprintf(sname, size, "fnt:%lx", h);
		h ++;
	} while (font_lookup(sname));
	sname[size - 1] = 0;
}

static int _free_sprite(const char *key);

static int luaB_free_sprites(lua_State *L) {
	sprites_free();
	return 0;
}

static int luaB_load_sprite(lua_State *L) {
	img_t img = NULL;
	_spr_t *sp;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];

	const char *fname = luaL_optstring(L, 1, NULL);
	const char *desc = luaL_optstring(L, 2, NULL);

	if (!fname)
		return 0;

	img = gfx_load_image((char*)fname);

	if (img)
		theme_img_scale(&img);

	if (img)
		img = gfx_display_alpha(img); /*speed up */

	if (!img)
		goto err;

	if (!desc || sprite_lookup(desc)) {
		key = sname;
		sprite_name(fname, sname, sizeof(sname));
	} else
		key = desc;
	sp = sprite_new(key, img);
	if (!sp)
		goto err;

	lua_pushstring(L, key);
	return 1;
err:
	game_res_err_msg(fname, debug_sw);
	gfx_free_image(img);
	return 0;
}

static int luaB_load_font(lua_State *L) {
	fnt_t fnt = NULL;
	_fnt_t *fn;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];
	struct game_theme *t = &game_theme;

	const char *fname = luaL_optstring(L, 1, NULL);
	int sz = luaL_optnumber(L, 2, t->font_size) * game_theme.scale;
	const char *desc = luaL_optstring(L, 3, NULL);
	if (!fname)
		return 0;

	fnt = fnt_load((char*)fname, sz);

	if (!fnt)
		return 0;

	if (!desc || font_lookup(desc)) {
		key = sname;
		font_name(fname, sname, sizeof(sname));
	} else
		key = desc;

	fn = font_new(key, fnt);
	if (!fn)
		goto err;

	lua_pushstring(L, key);
	return 1;
err:
	fnt_free(fnt);
	return 0;
}

static int luaB_text_size(lua_State *L) {
	_fnt_t *fn;
	int w = 0, h = 0;

	const char *font = luaL_optstring(L, 1, NULL);
	const char *text = luaL_optstring(L, 2, NULL);

	if (!font)
		return 0;

	fn = font_lookup(font);
	
	if (!fn)
		return 0;
	if (!text) {
		w = 0;
		h = ceil((float)fnt_height(fn->fnt) / game_theme.scale);
	} else {
		txt_size(fn->fnt, text, &w, &h);
		w = ceil((float)w / game_theme.scale);
		h = ceil((float)h / game_theme.scale);
	}
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}

static int luaB_font_size_scaled(lua_State *L) {
	int sz = luaL_optnumber(L, 1, game_theme.font_size);
	lua_pushnumber(L, FONT_SZ(sz));
	return 1;
}

static int luaB_text_sprite(lua_State *L) {
	img_t img = NULL;
	_spr_t *sp;
	_fnt_t *fn;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];

	const char *font = luaL_optstring(L, 1, NULL);
	const char *text = luaL_optstring(L, 2, NULL);
	char txtkey[32];
	const char *color = luaL_optstring(L, 3, NULL);
	int style = luaL_optnumber(L, 4, 0);
	const char *desc = luaL_optstring(L, 5, NULL);

	color_t col = { .r = game_theme.fgcol.r, .g = game_theme.fgcol.g, .b = game_theme.fgcol.b };

	if (!font)
		return 0;
	if (color)
		gfx_parse_color (color, &col);
	if (!text)
		text = "";

	fn = font_lookup(font);

	if (!fn)
		return 0;

	fnt_style(fn->fnt, style);

	img = fnt_render(fn->fnt, text, col);
	if (img)
		img = gfx_display_alpha(img); /*speed up */

	if (!img)
		return 0;
	
	if (!desc || sprite_lookup(desc)) {
		key = sname;
		strncpy(txtkey, text, sizeof(txtkey));
		txtkey[sizeof(txtkey) - 1] = 0;
		sprite_name(txtkey, sname, sizeof(sname));
	} else
		key = desc;
	
	sp = sprite_new(key, img);

	if (!sp)
		goto err;

	lua_pushstring(L, key);
	return 1;
err:
	gfx_free_image(img);
	return 0;
}

static img_t grab_sprite(const char *dst, int *xoff, int *yoff)
{
	img_t d;
	if (DIRECT_MODE && !strcmp(dst, "screen")) {
		d = gfx_screen(NULL);
		*xoff = game_theme.xoff;
		*yoff = game_theme.yoff;
	} else {
		*xoff = 0;
		*yoff = 0;
		d = cache_lookup(gfx_image_cache(), dst);
	}
	return d;
}


static int luaB_sprite_size(lua_State *L) {
	img_t s = NULL;
	float v;
	int w, h;
	int xoff, yoff;
	const char *src = luaL_optstring(L, 1, NULL);
	if (!src)
		return 0;
	s = grab_sprite(src, &xoff, &yoff);
	if (!s)
		return 0;
	
	v = game_theme.scale;

	w = ceil ((float)(gfx_img_w(s) - xoff * 2) / v);
	h = ceil ((float)(gfx_img_h(s) - yoff * 2) / v);

	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}

#define BLIT_COPY 0
#define BLIT_DRAW 1
#define BLIT_COMPOSE 2

static int luaB_blit_sprite(lua_State *L, int mode) {
	img_t s, d;
	img_t img2 = NULL;
	float v;
	const char *src = luaL_optstring(L, 1, NULL);
	int x = luaL_optnumber(L, 2, 0);
	int y = luaL_optnumber(L, 3, 0);
	int w = luaL_optnumber(L, 4, -1);
	int h = luaL_optnumber(L, 5, -1);
	const char *dst = luaL_optstring(L, 6, NULL);
	int xx = luaL_optnumber(L, 7, 0);
	int yy = luaL_optnumber(L, 8, 0);
	int alpha = luaL_optnumber(L, 9, 255);
	int xoff = 0, yoff = 0;
	int xoff0 = 0, yoff0 = 0;
	if (!src || !dst)
		return 0;

	s = grab_sprite(src, &xoff0, &yoff0);

	d = grab_sprite(dst, &xoff, &yoff);	

	if (!s || !d)
		return 0;

	v = game_theme.scale;

	if (v != 1.0f) {
		x *= v;
		y *= v;
		if (w != -1)
			w = ceil(w * v);
		if (h != -1)
			h = ceil(h * v);
		xx *= v;
		yy *= v;
	}

	if (w == -1)
		w = gfx_img_w(s) - 2 * xoff0;
	if (h == -1)
		h = gfx_img_h(s) - 2 * yoff0;

	game_pict_modify(d);

	if (alpha != 255) {
		img2 = gfx_alpha_img(s, alpha);
		if (img2)
			s = img2;
	}
	gfx_clip(game_theme.xoff, game_theme.yoff, game_theme.w - 2*game_theme.xoff, game_theme.h - 2*game_theme.yoff);

	switch (mode) {
	case BLIT_DRAW:
		gfx_draw_from(s, x + xoff0, y + yoff0, w, h, d, xx + xoff, yy + yoff);
		break;
	case BLIT_COPY:
		gfx_copy_from(s, x + xoff0, y + yoff0, w, h, d, xx + xoff, yy + yoff);
		break;
	case BLIT_COMPOSE:
		gfx_compose_from(s, x + xoff0, y + yoff0, w, h, d, xx + xoff, yy + yoff);
		break;
	default:
		break;
	}

	gfx_noclip();
	gfx_free_image(img2);
	lua_pushboolean(L, 1);
	return 1;
}


static int luaB_draw_sprite(lua_State *L) 
{
	return luaB_blit_sprite(L, BLIT_DRAW);
}

static int luaB_copy_sprite(lua_State *L) 
{
	return luaB_blit_sprite(L, BLIT_COPY);
}

static int luaB_compose_sprite(lua_State *L) 
{
	return luaB_blit_sprite(L, BLIT_COMPOSE);
}

static int luaB_alpha_sprite(lua_State *L) {
	_spr_t *sp;
	img_t s;
	img_t img2 = NULL;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];

	const char *src = luaL_optstring(L, 1, NULL);
	int alpha = luaL_optnumber(L, 2, 255);
	const char *desc = luaL_optstring(L, 3, NULL);

	if (!src)
		return 0;

	s = cache_lookup(gfx_image_cache(), src);
	if (!s)
		return 0;
	
	img2 = gfx_alpha_img(s, alpha);
	if (!img2)
		return 0;

	if (!desc || sprite_lookup(desc)) {
		key = sname;
		sprite_name(src, sname, sizeof(sname));
	} else
		key = desc;

	sp = sprite_new(key, img2);
	if (!sp)
		goto err;
	lua_pushstring(L, sname);
	return 1;
err:
	gfx_free_image(img2);
	return 0;
}

static int luaB_colorkey_sprite(lua_State *L) {
	img_t s;
	color_t  col;

	const char *src = luaL_optstring(L, 1, NULL);
	const char *color = luaL_optstring(L, 2, NULL);

	if (color)
		gfx_parse_color(color, &col);

	if (!src)
		return 0;
	s = cache_lookup(gfx_image_cache(), src);
	if (!s)
		return 0;
	if (color)
		gfx_set_colorkey(s, col);
	else 
		gfx_unset_colorkey(s);
	return 0;
}

static int luaB_dup_sprite(lua_State *L) {
	_spr_t *sp;
	img_t s;
	img_t img2 = NULL;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];
	const char *src = luaL_optstring(L, 1, NULL);
	const char *desc = luaL_optstring(L, 2, NULL);

	if (!src)
		return 0;

	s = cache_lookup(gfx_image_cache(), src);
	if (!s)
		return 0;

	img2 = gfx_alpha_img(s, 255);

	if (!img2)
		return 0;

	if (!desc || sprite_lookup(desc)) {
		key = sname;
		sprite_name(src, sname, sizeof(sname));
	} else
		key = desc;

	sp = sprite_new(key, img2);
	if (!sp)
		goto err;
	lua_pushstring(L, sname);
	return 1;
err:
	gfx_free_image(img2);
	return 0;
}

static int luaB_scale_sprite(lua_State *L) {
	_spr_t *sp;
	img_t s;
	img_t img2 = NULL;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];

	const char *src = luaL_optstring(L, 1, NULL);
	float xs = luaL_optnumber(L, 2, 0);
	float ys = luaL_optnumber(L, 3, 0);
	int smooth = lua_toboolean(L, 4);
	const char *desc = luaL_optstring(L, 5, NULL);

	if (!src)
		return 0;

	s = cache_lookup(gfx_image_cache(), src);
	if (!s)
		return 0;

	if (xs == 0)
		xs = 1.0f;

	if (ys == 0)
		ys = xs;

	img2 = gfx_scale(s, xs, ys, smooth);

	if (!img2)
		return 0;

	if (!desc || sprite_lookup(desc)) {
		key = sname;
		sprite_name(src, sname, sizeof(sname));
	} else
		key = desc;

	sp = sprite_new(key, img2);
	if (!sp)
		goto err;
	lua_pushstring(L, sname);
	return 1;
err:
	gfx_free_image(img2);
	return 0;
}


static int luaB_rotate_sprite(lua_State *L) {
	_spr_t *sp;
	img_t s;
	img_t img2 = NULL;
	const char *key;
	char sname[sizeof(unsigned long) * 2 + 16];

	const char *src = luaL_optstring(L, 1, NULL);
	float angle = luaL_optnumber(L, 2, 1.0f);
	int smooth = lua_toboolean(L, 3);
	const char *desc = luaL_optstring(L, 4, NULL);

	if (!src)
		return 0;

	s = cache_lookup(gfx_image_cache(), src);
	if (!s)
		return 0;
	img2 = gfx_rotate(s, angle, smooth);

	if (!img2)
		return 0;

	if (!desc || sprite_lookup(desc)) {
		key = sname;
		sprite_name(src, sname, sizeof(sname));
	} else
		key = desc;

	sp = sprite_new(key, img2);
	if (!sp)
		goto err;
	lua_pushstring(L, sname);
	return 1;
err:
	gfx_free_image(img2);
	return 0;
}

static int luaB_fill_sprite(lua_State *L) {
	img_t d;
	float v;
	const char *dst = luaL_optstring(L, 1, NULL);
	int x = luaL_optnumber(L, 2, 0);
	int y = luaL_optnumber(L, 3, 0);
	int w = luaL_optnumber(L, 4, -1);
	int h = luaL_optnumber(L, 5, -1);
	const char *color = luaL_optstring(L, 6, NULL);
	int xoff = 0, yoff = 0;
	color_t  col = { .r = game_theme.bgcol.r, .g = game_theme.bgcol.g, .b = game_theme.bgcol.b };
	if (!dst)
		return 0;

	d = grab_sprite(dst, &xoff, &yoff);

	if (color)
		gfx_parse_color(color, &col);

	if (!d)
		return 0;

	v = game_theme.scale;

	if (v != 1.0f) {
		x *= v;
		y *= v;
		if (w != -1)
			w = ceil(w * v);
		if (h != -1)
			h = ceil(h * v);
	}
	if (w == -1)
		w = gfx_img_w(d) - 2 * xoff;
	if (h == -1)
		h = gfx_img_h(d) - 2 * yoff;
	game_pict_modify(d);
	gfx_clip(game_theme.xoff, game_theme.yoff, game_theme.w - 2*game_theme.xoff, game_theme.h - 2*game_theme.yoff);
	gfx_img_fill(d, x + xoff, y + yoff, w, h, col);
	gfx_noclip();
	lua_pushboolean(L, 1);
	return 1;
}

static int luaB_pixel_sprite(lua_State *L) {
	img_t d;
	float v;
	int rc, w, h;
	color_t  col = { .r = game_theme.bgcol.r, .g = game_theme.bgcol.g, .b = game_theme.bgcol.b, .a = 255 };
	const char *dst = luaL_optstring(L, 1, NULL);
	int x = luaL_optnumber(L, 2, 0);
	int y = luaL_optnumber(L, 3, 0);
	const char *color = luaL_optstring(L, 4, NULL);
	int alpha = luaL_optnumber(L, 5, 255);
	int xoff = 0, yoff = 0;

	if (!dst)
		return 0;

	d = grab_sprite(dst, &xoff, &yoff);

	if (color)
		gfx_parse_color(color, &col);

	if (!d)
		return 0;

	w = gfx_img_w(d) - 2 * xoff;
	h = gfx_img_h(d) - 2 * yoff;

	v = game_theme.scale;

	if (v != 1.0f) {
		x *= v;
		y *= v;
	}

	if (color) {
		if (x < 0 || y < 0 || x >= w || y >= h)
			return 0;
		game_pict_modify(d);
		col.a = alpha;
		rc = gfx_set_pixel(d, x + xoff, y + yoff, col);
	} else {
		rc = gfx_get_pixel(d, x + xoff, y + yoff, &col);
	}

	if (rc)
		return 0;

	lua_pushnumber(L, col.r);
	lua_pushnumber(L, col.g);
	lua_pushnumber(L, col.b);
	lua_pushnumber(L, col.a);
	return 4;
}

static int _free_sprite(const char *key)
{
	_spr_t *sp;
	if (!key)
		return -1;
	sp = sprite_lookup(key);

	if (!sp)
		return -1;

	cache_forget(gfx_image_cache(), sp->img);
	cache_shrink(gfx_image_cache());

	list_del(&sp->list);
	free(sp->name); free(sp);
	return 0;
}

static int luaB_free_sprite(lua_State *L) {
	const char *key = luaL_optstring(L, 1, NULL);
	if (_free_sprite(key))
		return 0;
	lua_pushboolean(L, 1);
	return 1;
}

static int luaB_free_font(lua_State *L) {
	const char *key = luaL_optstring(L, 1, NULL);
	_fnt_t *fn;
	if (!key)
		return 0;

	fn = font_lookup(key);
	if (!fn)
		return 0;

	list_del(&fn->list);
	fnt_free(fn->fnt);
	free(fn->name); free(fn);
	lua_pushboolean(L, 1);
	return 1;
}

extern int theme_setvar(char *name, char *val);
extern char *theme_getvar(const char *name);

static int luaB_theme_var(lua_State *L) {
	const char *var = luaL_optstring(L, 1, NULL);
	const char *val = luaL_optstring(L, 2, NULL);
	if (var && !val) { /* get */
		char *p = theme_getvar(var);
		if (p) {
			lua_pushstring(L, p);
			free(p);
			return 1;
		}
		return 0;
	}
	if (!val || !var)
		return 0;
	if (!game_own_theme)
		return 0;
	if (!opt_owntheme)
		return 0;
	if (!strcmp(var, "scr.w") ||
		!strcmp(var, "scr.h")) /* filter resolution */
		return 0;
	if (!theme_setvar((char*)var, (char*)val)) {
		if (strcmp(var, "win.scroll.mode")) /* let change scroll mode w/o theme reload */
			game_theme_changed = 2;
	}
	return 0;
}

static int luaB_theme_name(lua_State *L) {
	char *name;
	if (game_own_theme && opt_owntheme) {
		if (game_own_theme == 2) {
			name = malloc(strlen(curtheme_dir[THEME_GAME]) + 2);
			if (!name)
				return 0;
			sprintf(name, ".%s", curtheme_dir[THEME_GAME]);
			lua_pushstring(L, name);
			free(name);
		} else {
			lua_pushstring(L, ".");
		}
	} else
		lua_pushstring(L, curtheme_dir[THEME_GLOBAL]);
	return 1;
}

static int luaB_stead_busy(lua_State *L) {
	static unsigned long busy_time = 0;
	int busy = lua_toboolean(L, 1);
	if (busy) {
		struct inp_event ev;
		int dirty = 0;
		memset(&ev, 0, sizeof(ev));
		while (input(&ev, 0) == AGAIN);
		if (ev.type == MOUSE_MOTION) {
			game_cursor(CURSOR_ON); /* to make all happy */
			dirty = 1;
		}
		if (!busy_time)
			busy_time = gfx_ticks();
		if (gfx_ticks() - busy_time >= 750 && menu_visible() != menu_wait) {
			game_menu(menu_wait);
			dirty = 1;
		}
		if (dirty)
			game_gfx_commit(0);
		return 0;
	}
	if (menu_visible() == menu_wait) {
		menu_toggle(-1);
	}
	busy_time = 0;
	return 0;
}

static int luaB_mouse_pos(lua_State *L) {
	int x = luaL_optnumber(L, 1, -1);
	int y = luaL_optnumber(L, 2, -1);
	int m;
	float v = game_theme.scale;
	if (x != -1 && y != -1) {
		x *= v;
		y *= v;
		gfx_warp_cursor(x + game_theme.xoff, y + game_theme.yoff);
		x = -1;
		y = -1;
	}
	m = gfx_cursor(&x, &y);
	x = (x - game_theme.xoff) / v;
	y = (y - game_theme.yoff) / v;
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, m);
	return 3;
}

static int luaB_finger_pos(lua_State *L) {
	int x, y;
	float pressure;
	float v = game_theme.scale;
	const char *finger = luaL_optstring(L, 1, NULL);
	if (!finger)
		return 0;
	if (finger_pos(finger, &x, &y, &pressure)) /* no finger */
		return 0;
	x = (x - game_theme.xoff) / v;
	y = (y - game_theme.yoff) / v;
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushnumber(L, pressure);
	return 3;
}

extern int mouse_filter_delay;

static int luaB_mouse_filter(lua_State *L) {
	int d = luaL_optnumber(L, 1, -1);
	int ov = mouse_filter_delay;
	if (d != -1)
		mouse_filter_delay = d;
	lua_pushnumber(L, ov);
	return 1;
}

static int luaB_mouse_show(lua_State *L) {
	int show = lua_toboolean(L, 1);
	int ov = game_cursor_show;
	if (lua_isboolean(L, 1))
		game_cursor_show = show;
	lua_pushboolean(L, ov);
	return 1;
}

static int luaB_get_ticks(lua_State *L) {
	lua_pushnumber(L, gfx_ticks());
	return 1;
}

static const luaL_Reg sprites_funcs[] = {
	{"instead_font_load", luaB_load_font},
	{"instead_font_free", luaB_free_font},
	{"instead_font_scaled_size", luaB_font_size_scaled},
	{"instead_sprite_load", luaB_load_sprite},
	{"instead_sprite_text", luaB_text_sprite},
	{"instead_sprite_free", luaB_free_sprite},
	{"instead_sprites_free", luaB_free_sprites},
	{"instead_sprite_draw", luaB_draw_sprite},
	{"instead_sprite_copy", luaB_copy_sprite},
	{"instead_sprite_compose", luaB_compose_sprite},
	{"instead_sprite_fill", luaB_fill_sprite},
	{"instead_sprite_dup", luaB_dup_sprite},
	{"instead_sprite_alpha", luaB_alpha_sprite},
	{"instead_sprite_colorkey", luaB_colorkey_sprite},
	{"instead_sprite_size", luaB_sprite_size},
	{"instead_sprite_scale", luaB_scale_sprite},
	{"instead_sprite_rotate", luaB_rotate_sprite},
	{"instead_sprite_text_size", luaB_text_size},
	{"instead_sprite_pixel", luaB_pixel_sprite},
	{"instead_theme_var", luaB_theme_var},
	{"instead_theme_name", luaB_theme_name},
	{"instead_ticks", luaB_get_ticks},
	{"instead_busy", luaB_stead_busy},
	{"instead_mouse_pos", luaB_mouse_pos},
	{"instead_mouse_filter", luaB_mouse_filter},
	{"instead_mouse_show", luaB_mouse_show},
	{"instead_finger_pos", luaB_finger_pos},
	{NULL, NULL}
};

static int sprites_done(void)
{
	sprites_free();
	return 0;
}

int instead_sprites_init(void)
{
	instead_hook_register(INSTEAD_HOOK_DONE, sprites_done);
	return instead_api_register(sprites_funcs);
}
