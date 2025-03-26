#include "PIDReportHandler.h"

char debugBuffer[100];

PIDReportHandler::PIDReportHandler()
{
	nextEID = 1;
	devicePaused = 0;
}

PIDReportHandler::~PIDReportHandler()
{
	FreeAllEffects();
}

uint8_t PIDReportHandler::GetNextFreeEffect(void)
{
	if (nextEID == MAX_EFFECTS)
		return 0;

	uint8_t id = nextEID++;

	while (g_EffectStates[nextEID].state != 0)
	{
		if (nextEID >= MAX_EFFECTS)
			break; // the last spot was taken
		nextEID++;
	}

	g_EffectStates[id].state = MEFFECTSTATE_ALLOCATED;

	return id;
}

void PIDReportHandler::StopAllEffects(void)
{
	for (uint8_t id = 0; id <= MAX_EFFECTS; id++)
		StopEffect(id);
}

void PIDReportHandler::StartEffect(uint8_t id)
{
	if (id > MAX_EFFECTS)
		return;
	g_EffectStates[id].state = MEFFECTSTATE_PLAYING;
	g_EffectStates[id].elapsedTime = 0;
	g_EffectStates[id].startTime = (uint64_t)millis();
}

void PIDReportHandler::StopEffect(uint8_t id)
{
	if (id > MAX_EFFECTS)
		return;
	g_EffectStates[id].state &= ~MEFFECTSTATE_PLAYING;
}

void PIDReportHandler::FreeEffect(uint8_t id)
{
	if (id > MAX_EFFECTS)
		return;
	g_EffectStates[id].state = MEFFECTSTATE_FREE;
	if (id < nextEID)
		nextEID = id;
	pidBlockLoad.ramPoolAvailable += SIZE_EFFECT;
}

void PIDReportHandler::FreeAllEffects(void)
{
	nextEID = 1;
	memset((void *)&g_EffectStates, 0, sizeof(g_EffectStates));
	pidBlockLoad.ramPoolAvailable = MEMORY_SIZE;
}

void PIDReportHandler::EffectOperation(USB_FFBReport_EffectOperation_Output_Data_t *data)
{
	if (data->operation == 1)
	{ // Start
		if (data->loopCount > 0)
			g_EffectStates[data->effectBlockIndex].duration *= data->loopCount;
		if (data->loopCount == 0xFF)
			g_EffectStates[data->effectBlockIndex].duration = USB_DURATION_INFINITE;
		StartEffect(data->effectBlockIndex);

		// sprintf(debugBuffer, "OStrt: %2d", data->effectBlockIndex);
		// Serial.println(debugBuffer);
	}
	else if (data->operation == 2)
	{ // StartSolo

		// Stop all first
		StopAllEffects();
		// Then start the given effect
		StartEffect(data->effectBlockIndex);

		// sprintf(debugBuffer, "OSolo: %2d", data->effectBlockIndex);
		// Serial.println(debugBuffer);
	}
	else if (data->operation == 3)
	{ // Stop
		StopEffect(data->effectBlockIndex);

		// sprintf(debugBuffer, "OStop: %2d", data->effectBlockIndex);
		// Serial.println(debugBuffer);
	}
	else
	{
		// sprintf(debugBuffer, "OUnkw: %2d", data->effectBlockIndex);
		// Serial.println(debugBuffer);
	}
}

void PIDReportHandler::BlockFree(USB_FFBReport_BlockFree_Output_Data_t *data)
{
	uint8_t eid = data->effectBlockIndex;

	if (eid == 0xFF)
	{ // all effects
		FreeAllEffects();
	}
	else
	{
		FreeEffect(eid);
	}
}

void PIDReportHandler::DeviceControl(USB_FFBReport_DeviceControl_Output_Data_t *data)
{

	uint8_t control = data->control;

	if (control == 0x01)
	{ // 1=Enable Actuators
		pidState.status |= 2;
	}
	else if (control == 0x02)
	{ // 2=Disable Actuators
		pidState.status &= ~(0x02);
	}
	else if (control == 0x03)
	{ // 3=Stop All Effects
		StopAllEffects();
	}
	else if (control == 0x04)
	{ //  4=Reset
		FreeAllEffects();
	}
	else if (control == 0x05)
	{ // 5=Pause
		devicePaused = 1;
	}
	else if (control == 0x06)
	{ // 6=Continue
		devicePaused = 0;
	}
	else if (control & (0xFF - 0x3F))
	{
	}
}

void PIDReportHandler::DeviceGain(USB_FFBReport_DeviceGain_Output_Data_t *data)
{
	deviceGain.gain = data->gain;
}

void PIDReportHandler::SetCustomForce(USB_FFBReport_SetCustomForce_Output_Data_t *data)
{
}

void PIDReportHandler::SetCustomForceData(USB_FFBReport_SetCustomForceData_Output_Data_t *data)
{
}

void PIDReportHandler::SetDownloadForceSample(USB_FFBReport_SetDownloadForceSample_Output_Data_t *data)
{
}

void PIDReportHandler::SetEffect(USB_FFBReport_SetEffect_Output_Data_t *data)
{
	volatile TEffectState *effect = &g_EffectStates[data->effectBlockIndex];

	effect->duration = data->duration;
	effect->directionX = data->directionX;
	effect->directionY = data->directionY;
	effect->effectType = data->effectType;
	effect->gain = data->gain;
	effect->enableAxis = data->enableAxis;

	// sprintf(debugBuffer, "SetEf: %2d (%d), duration: %d", data->effectBlockIndex, effect->effectType, effect->duration);
	// Serial.println(debugBuffer);
}

void PIDReportHandler::SetEnvelope(USB_FFBReport_SetEnvelope_Output_Data_t *data, volatile TEffectState *effect)
{
	effect->attackLevel = data->attackLevel;
	effect->fadeLevel = data->fadeLevel;
	effect->attackTime = data->attackTime;
	effect->fadeTime = data->fadeTime;
}

void PIDReportHandler::SetCondition(USB_FFBReport_SetCondition_Output_Data_t *data, volatile TEffectState *effect)
{
	uint8_t axis = data->parameterBlockOffset;
	effect->conditions[axis].cpOffset = data->cpOffset;
	effect->conditions[axis].positiveCoefficient = data->positiveCoefficient;
	effect->conditions[axis].negativeCoefficient = data->negativeCoefficient;
	effect->conditions[axis].positiveSaturation = data->positiveSaturation;
	effect->conditions[axis].negativeSaturation = data->negativeSaturation;
	effect->conditions[axis].deadBand = data->deadBand;
	effect->conditionBlocksCount++;
}

void PIDReportHandler::SetPeriodic(USB_FFBReport_SetPeriodic_Output_Data_t *data, volatile TEffectState *effect)
{
	effect->magnitude = data->magnitude;
	effect->offset = data->offset;
	effect->phase = data->phase;
	effect->period = data->period;

    //Idx /   Mag /   Off /   Phs /  Per
	
	//  1 /     0 /  -478 /     0 /  1000
	
	// 	5 /   900 /     0 /     0 /    63
	//  1 /     0 /  2519 /     0 /  1000

	// 	5 /   900 /     0 /     0 /    84
	//  1 /   350 /  6969 /     0 /  1838
	//  5 /   900 /     0 /     0 /    85
	//  1 /   350 /  6864 /     0 /  1840
	//  5 /   900 /     0 /     0 /    85
	//  1 /   350 /  6841 /     0 /  1842

	//  1 /   350 /  -848 /     0 /   913
	//  6 /  1000 /     0 /     0 /   204
	//  1 /   350 /  -833 /     0 /   922
	//  6 /  1000 /     0 /     0 /   205
	//  1 /   350 /  -813 /     0 /   929
	//  6 /  1000 /     0 /     0 /   206
	//  1 /   350 /  -810 /     0 /   933
	//  6 /  1000 /     0 /     0 /   207

	// sprintf(debugBuffer, "%2d / %5d / %5d / %5d / %5d", data->effectBlockIndex, effect->magnitude, effect->offset, effect->phase, effect->period);
	// Serial.println(debugBuffer);
}

void PIDReportHandler::SetConstantForce(USB_FFBReport_SetConstantForce_Output_Data_t *data, volatile TEffectState *effect)
{
	//  ReportPrint(*effect);
	effect->magnitude = data->magnitude;
}

void PIDReportHandler::SetRampForce(USB_FFBReport_SetRampForce_Output_Data_t *data, volatile TEffectState *effect)
{
	effect->startMagnitude = data->startMagnitude;
	effect->endMagnitude = data->endMagnitude;
}

void PIDReportHandler::CreateNewEffect(USB_FFBReport_CreateNewEffect_Feature_Data_t *inData)
{
	pidBlockLoad.reportId = 6;
	pidBlockLoad.effectBlockIndex = GetNextFreeEffect();

	if (pidBlockLoad.effectBlockIndex == 0)
	{
		pidBlockLoad.loadStatus = 2; // 1=Success,2=Full,3=Error
	}
	else
	{
		pidBlockLoad.loadStatus = 1; // 1=Success,2=Full,3=Error

		volatile TEffectState *effect = &g_EffectStates[pidBlockLoad.effectBlockIndex];

		memset((void *)effect, 0, sizeof(TEffectState));
		effect->state = MEFFECTSTATE_ALLOCATED;
		pidBlockLoad.ramPoolAvailable -= SIZE_EFFECT;
	}
}

void PIDReportHandler::UnpackUsbData(uint8_t *data, uint16_t len)
{
    // for (uint8_t i = 0; i < len; i++)
    // {
	// 	sprintf(debugBuffer, "%3d / ", data[i]);
	// 	Serial.print(debugBuffer);
    // }
	// Serial.println();

	uint8_t reportId = data[0];
	uint8_t effectId = data[1];

	if (reportId == 1) {
		SetEffect((USB_FFBReport_SetEffect_Output_Data_t *)data);
	} else if (reportId == 10) {
		EffectOperation((USB_FFBReport_EffectOperation_Output_Data_t *)data);
	} else if (reportId == 4) {
		SetPeriodic((USB_FFBReport_SetPeriodic_Output_Data_t *)data, &g_EffectStates[effectId]);
	} else if (reportId == 5) {
		SetConstantForce((USB_FFBReport_SetConstantForce_Output_Data_t *)data, &g_EffectStates[effectId]);
	} else if (reportId == 2) {
		SetEnvelope((USB_FFBReport_SetEnvelope_Output_Data_t *)data, &g_EffectStates[effectId]);
	} else if (reportId == 3) {
		SetCondition((USB_FFBReport_SetCondition_Output_Data_t *)data, &g_EffectStates[effectId]);
	} else if (reportId == 6) {
		SetRampForce((USB_FFBReport_SetRampForce_Output_Data_t *)data, &g_EffectStates[effectId]);
	} else if (reportId == 7) {
		SetCustomForceData((USB_FFBReport_SetCustomForceData_Output_Data_t *)data);
	} else if (reportId == 8) {
		SetDownloadForceSample((USB_FFBReport_SetDownloadForceSample_Output_Data_t *)data);
	} else if (reportId == 11) {
		BlockFree((USB_FFBReport_BlockFree_Output_Data_t *)data);
	} else if (reportId == 12) {
		DeviceControl((USB_FFBReport_DeviceControl_Output_Data_t *)data);
	} else if (reportId == 13) {
		DeviceGain((USB_FFBReport_DeviceGain_Output_Data_t *)data);
	} else if (reportId == 14) {
		SetCustomForce((USB_FFBReport_SetCustomForce_Output_Data_t *)data);
	}
}

uint8_t *PIDReportHandler::getPIDPool()
{
	FreeAllEffects();

	pidPoolReport.reportId = 7;
	pidPoolReport.ramPoolSize = MEMORY_SIZE;
	pidPoolReport.maxSimultaneousEffects = MAX_EFFECTS;
	pidPoolReport.memoryManagement = 3;
	return (uint8_t *)&pidPoolReport;
}

uint8_t *PIDReportHandler::getPIDBlockLoad()
{
	return (uint8_t *)&pidBlockLoad;
}

uint8_t *PIDReportHandler::getPIDStatus()
{
	return (uint8_t *)&pidState;
}