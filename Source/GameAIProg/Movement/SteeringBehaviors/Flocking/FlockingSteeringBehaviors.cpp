#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"


//*******************
//COHESION (FLOCKING)
SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput Steering{};
	
	Steering.LinearVelocity = pFlock->GetAverageNeighborPos() - pAgent.GetPosition();
	return Steering;
	
}

//*********************
//SEPARATION (FLOCKING)

SteeringOutput Separation::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering{};

	for (const auto& neighbour : pFlock->GetNeighbors())
	{
		FVector2D seperation {Agent.GetPosition() - neighbour->GetPosition()};
		float distance = FVector2D::Distance(Agent.GetPosition(), neighbour->GetPosition());
		
		if (distance > 0.01f)
			Steering.LinearVelocity += seperation / distance;
	}
	
	return Steering;
}

//*************************
//VELOCITY MATCH (FLOCKING)

SteeringOutput VelocityMatch::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{	
	SteeringOutput steering{};
	steering.LinearVelocity = pFlock->GetAverageNeighborVelocity();
	return steering;
}
