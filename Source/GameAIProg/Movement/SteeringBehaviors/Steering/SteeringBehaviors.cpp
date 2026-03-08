#include "SteeringBehaviors.h"

#include "VectorTypes.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "Math/UnitConversion.h"

//SEEK
//*******
// TODO: Do the Week01 assignment :^)

SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent & Agent)
{
	SteeringOutput Steering{};
	Steering.LinearVelocity = Target.Position - Agent.GetPosition();
	Steering.LinearVelocity.Normalize();


	if (m_bDebugDraw)
	{
		constexpr float DebugScale = 100.f;
		DrawDebugLine(Agent.GetWorld(),
					  FVector(Agent.GetPosition().X, Agent.GetPosition().Y, 1),
					  FVector(Agent.GetPosition().X + Steering.LinearVelocity.X * DebugScale,
							  Agent.GetPosition().Y + Steering.LinearVelocity.Y * DebugScale, 1),
					  FColor::Blue, false, 0.f, 0, 2.f);
	}


	

	return Steering;
}

SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering;
	Steering.LinearVelocity = Target.Position +  Agent.GetPosition();
	Steering.LinearVelocity.Normalize();
	
	
	if (m_bDebugDraw)
	{
		DrawDebugLine(Agent.GetWorld(),
					  FVector(Agent.GetPosition().X, Agent.GetPosition().Y, 1),
					  FVector(Agent.GetPosition().X + Steering.LinearVelocity.X * 100.f,
							  Agent.GetPosition().Y + Steering.LinearVelocity.Y * 100.f, 1),
					  FColor::Red, false, 0.f, 0, 2.f);
	}

	
	return Steering;
}

SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	constexpr int segments{16};

	
	SteeringOutput Steering;
	FVector2D toTarget { Target.Position - Agent.GetPosition()};
	float Distance = toTarget.Size();
	
	
	if (Distance < m_TargetRadius)
	{
		Steering.LinearVelocity = FVector2D::ZeroVector;
		return Steering;
	}

	
	
	float TargetSpeed = Agent.GetMaxLinearSpeed();
	if (Distance < m_SlowRadius)
	{
		TargetSpeed *= Distance / m_SlowRadius;
	}
	

	
	Steering.LinearVelocity = toTarget / Distance * TargetSpeed;
	

	if (m_bDebugDraw)
	{
		DrawDebugCircle(Agent.GetWorld(),
						FVector(Agent.GetPosition().X, Agent.GetPosition().Y, 1),
						m_SlowRadius, 16, FColor::Red);
		DrawDebugCircle(Agent.GetWorld(),
						FVector(Agent.GetPosition().X, Agent.GetPosition().Y, 1),
						m_TargetRadius, 16, FColor::Green);
	}

	return Steering;
}

SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Steering{};
    float DistToTarget{ float((Target.Position - Agent.GetPosition()).Length()) };
    float Time{ DistToTarget / Agent.GetMaxLinearSpeed() };
    FVector2D PredictedPosition{ Target.Position + Target.LinearVelocity * Time };
    Steering.LinearVelocity = PredictedPosition - Agent.GetPosition();
    
    return Steering;
}
SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	const FVector2D Center{ Agent.GetPosition() + Agent.GetLinearVelocity().GetSafeNormal() * m_OffsetDistance };
	float AngleRad{ ((rand() % (m_MaxAngleChange * 2)) - m_MaxAngleChange) / 180.f * PI };
	m_WanderAngle += AngleRad;
    
	const FVector2D Pos{ Center.X + cos(m_WanderAngle) * m_Radius, 
						 Center.Y + sin(m_WanderAngle) * m_Radius };
	Target.Position = Pos;
    

	if (m_bDebugDraw)
	{
		DrawDebugCircle(Agent.GetWorld(), FVector(Center.X, Center.Y, 1), m_Radius, 16, FColor::Yellow);
		DrawDebugLine(Agent.GetWorld(),
					  FVector(Agent.GetPosition().X, Agent.GetPosition().Y, 1),
					  FVector(Pos.X, Pos.Y, 1), FColor::Orange);
	}
    
	return Seek::CalculateSteering(DeltaT, Agent);
}

SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering{};

	FVector2D ToTarget = Target.Position - Agent.GetPosition();
	float DistToTarget = ToTarget.Size();
	float Time = DistToTarget / Agent.GetMaxLinearSpeed();

	FVector2D Predicted   = Target.Position + Target.LinearVelocity * Time;
	Steering.LinearVelocity = Agent.GetPosition() - Predicted ;


	if (!Steering.LinearVelocity.IsNearlyZero())
		Steering.LinearVelocity.Normalize();
	
	Steering.LinearVelocity *= Agent.GetMaxLinearSpeed(); 
	Steering.IsValid = true;
	
	if (m_bDebugDraw)
	{
		DrawDebugLine(Agent.GetWorld(),
					  FVector(Agent.GetPosition().X, Agent.GetPosition().Y, 1),
					  FVector(Predicted.X, Predicted.Y, 1), FColor::Purple);
	}
	return Steering;
}