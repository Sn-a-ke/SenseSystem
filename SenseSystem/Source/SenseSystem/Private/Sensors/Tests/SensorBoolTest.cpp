//Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "Sensors/Tests/SensorBoolTest.h"


EUpdateReady USensorBoolTest::GetReadyToTest()
{
	return EUpdateReady::Ready;
}

bool USensorBoolTest::PreTest()
{
	Super::PreTest();
	return true;
}

ESenseTestResult USensorBoolTest::RunTest(FSensedStimulus& SensedStimulus) const
{
	return TestResult;
}
