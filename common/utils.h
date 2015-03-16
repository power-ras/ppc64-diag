/*
 * Copyright (C) 2015 IBM Corporation
 */
#ifndef UTILS_H
#define UTILS_H

FILE	*spopen(char **, pid_t *);
int	spclose(FILE *, pid_t);

#endif
