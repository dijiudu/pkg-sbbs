/* truncsp.c */

/* Static functions to truncate white-space chars off end of ASCIIZ strings */

/* $Id: truncsp.c,v 1.3 2004/08/04 04:44:27 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

/****************************************************************************/
/* Truncates all white-space chars off end of 'str'							*/
/****************************************************************************/
static char* truncsp(char* str)
{
	unsigned c;

	c=strlen(str);
	while(c && (str[c-1]==' ' || str[c-1]=='\t' || str[c-1]=='\r' || str[c-1]=='\n')) c--;
	str[c]=0;

	return(str);
}

/****************************************************************************/
/* Truncates carriage-return and line-feed chars off end of 'str'			*/
/****************************************************************************/
static char* truncnl(char* str)
{
	unsigned c;

	c=strlen(str);
	while(c && (str[c-1]=='\r' || str[c-1]=='\n')) c--;
	str[c]=0;

	return(str);
}
