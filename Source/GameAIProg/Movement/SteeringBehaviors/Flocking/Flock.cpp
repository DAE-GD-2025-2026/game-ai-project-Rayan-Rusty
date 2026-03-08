#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "Movement/SteeringBehaviors/SpacePartitioning/SpacePartitioning.h"
#include "Shared/ImGuiHelpers.h"


Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld)
	: pWorld{pWorld}
	, FlockSize{ FlockSize }
	, m_pAgentToEvade{pAgentToEvade}
{
	
	
	float currentWorldSize{ WorldSize};
	
	pPartitionedSpace = std::make_unique<CellSpace>(pWorld , currentWorldSize , currentWorldSize , 10 , 10 , FlockSize);
	
	pSeparationBehavior = std::make_unique<Separation>(this);
	pCohesionBehavior   = std::make_unique<Cohesion>(this);
	pVelMatchBehavior   = std::make_unique<VelocityMatch>(this);
	
	pSeekBehavior       = std::make_unique<Seek>();
	pWanderBehavior     = std::make_unique<Wander>();
	pEvadeBehavior      = std::make_unique<Evade>();	
	

	std::vector<BlendedSteering::WeightedBehavior> WeightedBehaviors;
	WeightedBehaviors.reserve(5);
	WeightedBehaviors.push_back({pSeparationBehavior.get(), 0.1f});
	WeightedBehaviors.push_back({pCohesionBehavior.get(), 0.2f});
	WeightedBehaviors.push_back({pVelMatchBehavior.get(), 0.2f});
	WeightedBehaviors.push_back({pSeekBehavior.get(), 0.3f});
	WeightedBehaviors.push_back({pWanderBehavior.get(), 0.2f});
	pBlendedSteering = std::make_unique<BlendedSteering>(WeightedBehaviors);
	pPrioritySteering = std::make_unique<PrioritySteering>(std::vector<ISteeringBehavior*>{ pEvadeBehavior.get(), pBlendedSteering.get() });
	
	Agents.SetNum(FlockSize);
	Neighbors.SetNum(FlockSize);
	NrOfNeighbors = 0;
    
	for (int i{ 0 }; i < FlockSize; ++i)
	{
		while (!Agents[i])
		{
			FVector2D RandPos{
				FMath::RandRange(-WorldSize * 0.5f, WorldSize * 0.5f),
				FMath::RandRange(-WorldSize * 0.5f, WorldSize * 0.5f)
			};
			Agents[i] = pWorld->SpawnActor<ASteeringAgent>(AgentClass, FVector{RandPos.X,RandPos.Y,90}, FRotator::ZeroRotator);
			
		}
		pPartitionedSpace->AddAgent(*Agents[i]);
	}
	
	for (int i{ 0 }; i < FlockSize; ++i)
	{
		if (Agents[i])
		{
			Agents[i]->SetSteeringBehavior(pPrioritySteering.get());
		}
	}
	
	
	FVector2D RandPos{
		FMath::RandRange(-WorldSize * 0.5f, WorldSize * 0.5f),
		FMath::RandRange(-WorldSize * 0.5f, WorldSize * 0.5f)
	};
	FVector EvadeSpawn{RandPos.X, RandPos.Y, 90.f};
	
	m_pAgentToEvade = pWorld->SpawnActor<ASteeringAgent>(AgentClass, EvadeSpawn , FRotator::ZeroRotator);
	if (m_pAgentToEvade)
		m_pAgentToEvade->SetSteeringBehavior(pWanderBehavior.get());


}

Flock::~Flock()
{
}

void Flock::Tick(float DeltaTime)
{
 // TODO: update the flock
 // TODO: for every agent:
  // TODO: register the neighbors for this agent (-> fill the memory pool with the neighbors for the currently evaluated agent)
  // TODO: update the agent (-> the steeringbehaviors use the neighbors in the memory pool)
  // TODO: trim the agent to the world



	 for (ASteeringAgent* Agent : Agents)
	 {
		if (bUseSpacePartitioning)
		{
			pPartitionedSpace->UpdateAgentCell(*Agent, Agent->GetPosition());
			pPartitionedSpace->RegisterNeighbors(*Agent , NeighborhoodRadius);
		}
	 	else
	 	{
	 		RegisterNeighbors(Agent);
	 	}
	 	Agent->Tick(DeltaTime);
	 	
	 }
	
	if (m_pAgentToEvade && pEvadeBehavior)
	{
		FTargetData Target;

		Target.Position = m_pAgentToEvade->GetPosition();
		Target.LinearVelocity = m_pAgentToEvade->GetLinearVelocity();
		Target.AngularVelocity = m_pAgentToEvade->GetAngularVelocity();
		Target.Orientation = m_pAgentToEvade->GetRotation();

		pEvadeBehavior->SetTarget(Target);
	}
	
	RenderNeighborhood();
	if (bUseSpacePartitioning && DebugRenderPartitions)
	{
		pPartitionedSpace->RenderCells();
	}
}

void Flock::RenderDebug()
{
	for (ASteeringAgent* agent : Agents)
	{
		agent->SetDebugRenderingEnabled(true);
	}
	
	
	if (!m_pAgentToEvade) 
		return;

	// Position slightly ahead of the evade agent to show direction
	FVector EvadePos = m_pAgentToEvade->GetActorLocation();
	FVector Forward = m_pAgentToEvade->GetActorForwardVector();
	FVector DebugCenter = EvadePos + Forward * 100.f; // 100 units in front

	// Draw three concentric circles around the evade agent
	DrawDebugCircle(
		pWorld, DebugCenter, 100.f, 32, FColor::Red, false, -1.f, 0, 2.f,
		FVector(0,1,0), FVector(1,0,0), true
	);

	DrawDebugCircle(
		pWorld, DebugCenter, 90.f, 32, FColor::Blue, false, -1.f, 0, 2.f,
		FVector(0,1,0), FVector(1,0,0), true
	);

	DrawDebugCircle(
		pWorld, DebugCenter, 80.f, 32, FColor::Red, false, -1.f, 0, 2.f,
		FVector(0,1,0), FVector(1,0,0), true
	);
}

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
#pragma region UI
	//UI
	{
		//Setup
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: place target");
		ImGui::Text("RMB: move cam.");
		ImGui::Text("Scrollwheel: zoom cam.");
		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();
		ImGui::Checkbox("Use Spatial Partitioning", &bUseSpacePartitioning);
		ImGui::Checkbox("Render Cells", &DebugRenderPartitions);
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		
		
		ImGui::Text("Flocking");
		ImGui::Spacing();

  // TODO: implement ImGUI checkboxes for debug rendering here

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Seperation" , pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight, 0.f , 1.f,
			[this](float Inval){pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight = Inval;} , "%.2f");
  // TODO: implement ImGUI sliders for steering behavior weights here
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Cohesion" , pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight, 0.f , 1.f,
	[this](float Inval){pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight = Inval;} , "%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Alignment" , pBlendedSteering->GetWeightedBehaviorsRef()[2].Weight, 0.f , 1.f,
[this](float Inval){pBlendedSteering->GetWeightedBehaviorsRef()[2].Weight = Inval;} , "%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek" , pBlendedSteering->GetWeightedBehaviorsRef()[3].Weight, 0.f , 1.f,
[this](float Inval){pBlendedSteering->GetWeightedBehaviorsRef()[3].Weight = Inval;} , "%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander" , pBlendedSteering->GetWeightedBehaviorsRef()[4].Weight, 0.f , 1.f,
[this](float Inval){pBlendedSteering->GetWeightedBehaviorsRef()[4].Weight = Inval;} , "%.2f");
		
		//End
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
 // TODO: Debugrender the neighbors for the first agent in the flock
	RegisterNeighbors(Agents[0]);
	
	size_t index{0};
	// draw neighbourhood circle
	DrawDebugCircle(pWorld, FVector(Agents[0]->GetPosition().X, Agents[0]->GetPosition().Y, 1), NeighborhoodRadius, 16, 
			FColor(0, 0, 0, 255), false, -1, 0, 0,
		FVector(0, 1, 0), FVector(1, 0, 0), false);
	
	// draw blue circle around first agent
	DrawDebugCircle(pWorld, FVector(Agents[0]->GetPosition().X, Agents[0]->GetPosition().Y, 1), 50, 16, 
			FColor(0, 0, 255, 255), false, -1, 0, 0,
		FVector(0, 1, 0), FVector(1, 0, 0), false);
	
	// draw purple circle around registered neighbors
	for (int i = 0; i < NrOfNeighbors; ++i)
	{
		ASteeringAgent* neighbor = Neighbors[i];
		DrawDebugCircle(pWorld, FVector(neighbor->GetPosition().X, neighbor->GetPosition().Y, 1), 50, 16, 
			FColor(200, 0, 255, 255), false, -1, 0, 0,
		FVector(0, 1, 0), FVector(1, 0, 0), false);
        
		++index;
		if (index >= NrOfNeighbors)
			break;
	}
}


void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
	if (bUseSpacePartitioning && pPartitionedSpace)
	{
		pPartitionedSpace->RegisterNeighbors(*pAgent, NeighborhoodRadius);

		NrOfNeighbors = pPartitionedSpace->GetNrOfNeighbors();
		const auto& SpatialNeighbors = pPartitionedSpace->GetNeighbors();

		for (int i = 0; i < NrOfNeighbors && i < Neighbors.Num(); ++i)
		{
			Neighbors[i] = SpatialNeighbors[i];
		}
	}
	else
	{
		NrOfNeighbors = 0;

		for (ASteeringAgent* other : Agents)
		{
			if (other == pAgent)
				continue;

			FVector2D diff = other->GetPosition() - pAgent->GetPosition();

			if (diff.Length() <= NeighborhoodRadius)
			{
				Neighbors[NrOfNeighbors] = other;
				NrOfNeighbors++;

				if (NrOfNeighbors >= Neighbors.Num())
					break;
			}
		}
	}
}

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;

	if (NrOfNeighbors == 0) return avgPosition;

	for (int i = 0; i < NrOfNeighbors; ++i)
	{
		avgPosition += Neighbors[i]->GetPosition();
	}
	
	avgPosition /= float(NrOfNeighbors);
	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;
	
	if (NrOfNeighbors == 0) return avgVelocity;

	for (int i = 0; i < NrOfNeighbors; ++i)
	{
		avgVelocity += Neighbors[i]->GetLinearVelocity();
	}

	avgVelocity /= float(NrOfNeighbors);
	return avgVelocity;
}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
	pSeekBehavior->SetTarget(Target);
}

