
#include "CombinedSteeringBehaviors.h"
#include <algorithm>
#include "../SteeringAgent.h"

BlendedSteering::BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors)
	:WeightedBehaviors(WeightedBehaviors)
{};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput blended{};
	float totalWeight = 0.f;

	for (const WeightedBehavior& wb : WeightedBehaviors)
	{
		if (!wb.pBehavior || wb.Weight <= 0.f)
			continue;

		// Get steering from this behavior
		SteeringOutput steering = wb.pBehavior->CalculateSteering(DeltaT, Agent);

		if (!steering.IsValid)
			continue;

		// Add weighted contribution
		blended.LinearVelocity += steering.LinearVelocity * wb.Weight;
		blended.AngularVelocity += steering.AngularVelocity * wb.Weight;

		totalWeight += wb.Weight;
	}

	// Normalize by total weight
	if (totalWeight > 0.f)
	{
		blended.LinearVelocity /= totalWeight;
		blended.AngularVelocity /= totalWeight;
		blended.IsValid = true;
	}
	
	blended.LinearVelocity =
		blended.LinearVelocity.GetClampedToMaxSize(Agent.GetMaxLinearSpeed());
	return blended;
}

float* BlendedSteering::GetWeight(ISteeringBehavior* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
		WeightedBehaviors.end(),
		[SteeringBehavior](const WeightedBehavior& Elem)
		{
			return Elem.pBehavior == SteeringBehavior;
		}
	);

	if(it!= WeightedBehaviors.end())
		return &it->Weight;
	
	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering = {}; 


	
	
	for (ISteeringBehavior* const pBehavior : m_PriorityBehaviors)
	{
		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		if (Steering.IsValid)
			break;

	}

	return Steering;
}