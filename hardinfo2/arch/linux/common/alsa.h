/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 Leandro A. F. Pereira <leandro@linuxmag.com.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

gchar *
computer_get_alsacards(Computer * computer)
{
    GSList *p;
    gchar *tmp = "";
    gint n = 0;

    if (computer->alsa) {
	for (p = computer->alsa->cards; p; p = p->next) {
	    AlsaCard *ac = (AlsaCard *) p->data;

	    tmp =
		g_strdup_printf("Audio Adapter#%d=%s\n%s", ++n,
				ac->friendly_name, tmp);
	}
    }

    return tmp;
}

static AlsaInfo *
computer_get_alsainfo(void)
{
    AlsaInfo *ai;
    AlsaCard *ac;
    FILE *cards;
    gchar buffer[128];

    cards = fopen("/proc/asound/cards", "r");
    if (!cards)
	return NULL;

    ai = g_new0(AlsaInfo, 1);

    while (fgets(buffer, 128, cards)) {
	gchar **tmp;

	ac = g_new0(AlsaCard, 1);

	tmp = g_strsplit(buffer, ":", 0);

	ac->friendly_name = g_strdup(tmp[1]);
	ai->cards = g_slist_append(ai->cards, ac);

	g_strfreev(tmp);
	fgets(buffer, 128, cards);	/* skip next line */
    }
    fclose(cards);

    return ai;
}
