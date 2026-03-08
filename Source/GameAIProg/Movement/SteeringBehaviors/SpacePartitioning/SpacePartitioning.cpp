#include "SpacePartitioning.h"

// --- Cell ---
// ------------
Cell::Cell(float Left, float Bottom, float Width, float Height)
{
	BoundingBox.Min = { Left, Bottom };
	BoundingBox.Max = { BoundingBox.Min.X + Width, BoundingBox.Min.Y + Height };
}

std::vector<FVector2D> Cell::GetRectPoints() const
{
	const float left = BoundingBox.Min.X;
	const float bottom = BoundingBox.Min.Y;
	const float width = BoundingBox.Max.X - BoundingBox.Min.X;
	const float height = BoundingBox.Max.Y - BoundingBox.Min.Y;

	std::vector<FVector2D> rectPoints =
	{
		{ left , bottom  },
		{ left , bottom + height  },
		{ left + width , bottom + height },
		{ left + width , bottom  },
	};

	return rectPoints;
}

// --- Partitioned Space ---
// -------------------------
CellSpace::CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities)
	: pWorld{pWorld}
	, SpaceWidth{Width}
	, SpaceHeight{Height}
	, NrOfRows{Rows}
	, NrOfCols{Cols}
	, NrOfNeighbors{0}
{
	Neighbors.SetNum(MaxEntities);
	
	//calculate bounds of a cell
	CellWidth = Width / Cols;
	CellHeight = Height / Rows;

	Cells.reserve(Rows * Cols);
	for (int row = 0; row < Rows; ++row)
	{
		for (int col  = 0; col < Cols; ++col )
		{
			float left {col * CellWidth - (Width / 2.f)};
			float bottom {row * CellHeight - (Height/ 2.f)};
			Cells.emplace_back(left, bottom , CellWidth, CellHeight); 
		}
	}	
	
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	
	int index {PositionToIndex(Agent.GetPosition())};
	Cells[index].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	int oldIndex {PositionToIndex(OldPos)};
	int newIndex{PositionToIndex(Agent.GetPosition())};

	if (oldIndex != newIndex)
	{
		auto& oldCellAgents = Cells[oldIndex].Agents;
		oldCellAgents.erase(std::remove(oldCellAgents.begin(), oldCellAgents.end(), &Agent), oldCellAgents.end());
		Cells[newIndex].Agents.push_back(&Agent);
	}
	
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	NrOfNeighbors = 0;
	FRect Rect;
	for (const Cell& cell : Cells)
	{
		if (!DoRectsOverlap(cell.BoundingBox , Rect))
			continue;

		for (ASteeringAgent* other : cell.Agents)
		{
			if (other == &Agent)
				continue;
			
			if (FVector2D::Distance(other->GetPosition(), Agent.GetPosition()) <= QueryRadius)
			{
				Neighbors[NrOfNeighbors++] = other;
				if (NrOfNeighbors >= Neighbors.Num()) break;
			}
		}
		
	}
	
}

void CellSpace::EmptyCells()
{
	for (Cell& c : Cells)
		c.Agents.clear();
}

void CellSpace::RenderCells() const
{
	for (const Cell& cell : Cells)
	{
		constexpr float scale{1.f};
		FVector min{cell.BoundingBox.Min.X ,  cell.BoundingBox.Min.Y , 100.f};	
		FVector max{cell.BoundingBox.Max.X ,  cell.BoundingBox.Max.Y , 100.f};	
		
		DrawDebugBox(pWorld , (min + max) * scale , (max - min) * scale 
			, FColor::Emerald , false , -1.f , 0, 5.f);
		
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{
	float relativeX = static_cast<float>(Pos.X + SpaceWidth * 0.5f);
	float relativeY = static_cast<float>(Pos.Y + SpaceHeight * 0.5f);
	
	int col {FMath::FloorToInt(relativeX / CellWidth)};
	int row {FMath::FloorToInt(relativeY / CellHeight)};
	
	
	col = FMath::Clamp(col , 0 , NrOfCols - 1);
	row = FMath::Clamp(row , 0, NrOfRows - 1);
	
	
	return (row * NrOfCols) + col;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{
	// Check if the rectangles are separated on either axis
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
    
	// If they are not separated, they must overlap
	return true;
}