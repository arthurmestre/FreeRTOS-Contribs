/*
 * ReadersWritersLock.h
 *
 *  Created on: 22/03/2018
 *      Author: PC
 */

#ifndef SOURCES_READERSWRITERSLOCK_H_
#define SOURCES_READERSWRITERSLOCK_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct
{
//	uint8_t name[20];
	SemaphoreHandle_t semLock;
	QueueHandle_t readSleepQueue;
	QueueHandle_t writeSleepQueue;
	uint8_t ww; //waiting writers
	uint8_t aw; //active writers
	uint8_t wr; //waiting readers
	uint8_t ar; //active readers
} Rwlock;

Rwlock *rwlock_create();
void rwlock_destroy(Rwlock *rwlock);
uint8_t rwlock_acquire_read(Rwlock *rwlock);
uint8_t rwlock_release_read(Rwlock *rwlock);
uint8_t rwlock_acquire_write(Rwlock *rwlock);
uint8_t rwlock_release_write(Rwlock *rwlock);


#endif /* SOURCES_READERSWRITERSLOCK_H_ */
