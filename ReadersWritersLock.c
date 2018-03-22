/*
 * ReadersWritersLock.c
 *
 *  Created on: 22/03/2018
 *      Author: PC
 */

#include "ReadersWritersLock.h"

Rwlock *rwlock_create()
{
	Rwlock *rwlock;

	rwlock = pvPortMalloc(sizeof(Rwlock));

	if(rwlock == NULL)
		return NULL;
	rwlock->semLock = xSemaphoreCreateMutex();

	if(rwlock->semLock == NULL)
	{
		vPortFree(rwlock);
		return NULL;
	}

	rwlock->readSleepQueue = xQueueCreate(5, sizeof(TaskHandle_t));
	if(rwlock->readSleepQueue == NULL)
	{
		vSemaphoreDelete(rwlock->semLock);
		vPortFree(rwlock);
		return NULL;
	}

	rwlock->writeSleepQueue = xQueueCreate(5, sizeof(TaskHandle_t));
	if(rwlock->writeSleepQueue == NULL)
	{
		vSemaphoreDelete(rwlock->semLock);
		vQueueDelete(rwlock->readSleepQueue);
		vPortFree(rwlock);
		return NULL;
	}

	rwlock->ww = 0;
	rwlock->wr = 0;
	rwlock->ar = 0;
	rwlock->aw = 0;

	return rwlock;
}

void rwlock_destroy(Rwlock *rwlock)
{
	if(rwlock == NULL)
		return;

	if(!xSemaphoreTake(rwlock->semLock, 2500))
	{
		return;
	}

	if(rwlock->aw == 0 && rwlock->ar == 0 && rwlock->ww == 0 && rwlock->wr == 0){
		vSemaphoreDelete(rwlock->semLock);
		vQueueDelete(rwlock->readSleepQueue);
		vQueueDelete(rwlock->writeSleepQueue);
		vPortFree(rwlock);
	}

}

uint8_t rwlock_acquire_read(Rwlock *rwlock)
{
	if(rwlock == NULL)
		return 0;

	if(!xSemaphoreTake(rwlock->semLock, 2500))
	{
		return 0;
	}

	while(rwlock->aw > 0){ // active writer
		rwlock->wr++; //waiting reader
		xQueueSendToBack(rwlock->readSleepQueue, xTaskGetCurrentTaskHandle(), 2500);
		xSemaphoreGive(rwlock->semLock);
	    ulTaskNotifyTake(pdTRUE, portMAX_DELAY ); /* Block indefinitely. */
		if(!xSemaphoreTake(rwlock->semLock, 2500))
			return 0;
		rwlock->wr--;
	}

	rwlock->ar++;
	xSemaphoreGive(rwlock->semLock);
	return 1;
}

uint8_t rwlock_release_read(Rwlock *rwlock)
{
	if(rwlock == NULL)
		return 0;

	if(!xSemaphoreTake(rwlock->semLock, 2500))
	{
		return 0;
	}

	rwlock->ar--;

	if(rwlock->ar == 0 && rwlock->ww > 0){
		TaskHandle_t writerHandle = NULL;
		if(!xQueueReceive(rwlock->writeSleepQueue, &writerHandle, 2500))
			return 0;
		xTaskNotifyGive(writerHandle);
	}
	xSemaphoreGive(rwlock->semLock);
	return 1;
}

uint8_t rwlock_acquire_write(Rwlock *rwlock)
{
	if(rwlock == NULL)
		return 0;

	if(!xSemaphoreTake(rwlock->semLock, 2500))
		return 0;

	while(rwlock->ar + rwlock->aw > 0){
		rwlock->ww++;
		xQueueSendToBack(rwlock->writeSleepQueue, xTaskGetCurrentTaskHandle(), 2500);
		xSemaphoreGive(rwlock->semLock);
	    ulTaskNotifyTake(pdTRUE, portMAX_DELAY ); /* Block indefinitely. */
		if(!xSemaphoreTake(rwlock->semLock, 2500))
			return 0;
		rwlock->ww--;
	}
	rwlock->ww++;
	xSemaphoreGive(rwlock->semLock);
	return 1;
}

uint8_t rwlock_release_write(Rwlock *rwlock)
{
	if(rwlock == NULL)
		return 0;

	if(!xSemaphoreTake(rwlock->semLock, 2500))
		return 0;

	rwlock->aw--;

	if(rwlock->ww > 0){
		TaskHandle_t writerHandle = NULL;
		if(!xQueueReceive(rwlock->writeSleepQueue, &writerHandle, 2500))
			return 0;
		xTaskNotifyGive(writerHandle);
	}else if(rwlock->wr > 0){
		TaskHandle_t readerHandle = NULL;
		while(xQueueReceive(rwlock->writeSleepQueue, &readerHandle, 2500))
		{
			xTaskNotifyGive(readerHandle);
		}
	}

	xSemaphoreGive(rwlock->semLock);
	return 1;
}
