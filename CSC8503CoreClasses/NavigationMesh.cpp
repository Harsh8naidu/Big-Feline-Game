#include "NavigationMesh.h"
#include "Assets.h"
#include "Maths.h"
#include <fstream>
#include <queue>
using namespace NCL;
using namespace CSC8503;
using namespace std;

NavigationMesh::NavigationMesh()
{
}

NavigationMesh::NavigationMesh(const std::string&filename)
{
	ifstream file(Assets::DATADIR + filename);

	int numVertices = 0;
	int numIndices	= 0;

	file >> numVertices;
	file >> numIndices;

	for (int i = 0; i < numVertices; ++i) {
		Vector3 vert;
		file >> vert.x;
		file >> vert.y;
		file >> vert.z;

		allVerts.emplace_back(vert);
	}

	allTris.resize(numIndices / 3);

	for (int i = 0; i < allTris.size(); ++i) {
		NavTri* tri = &allTris[i];
		file >> tri->indices[0];
		file >> tri->indices[1];
		file >> tri->indices[2];

		tri->centroid = allVerts[tri->indices[0]] +
			allVerts[tri->indices[1]] +
			allVerts[tri->indices[2]];

		tri->centroid = allTris[i].centroid / 3.0f;

		tri->triPlane = Plane::PlaneFromTri(allVerts[tri->indices[0]],
			allVerts[tri->indices[1]],
			allVerts[tri->indices[2]]);

		tri->area = Maths::AreaofTri3D(allVerts[tri->indices[0]], allVerts[tri->indices[1]], allVerts[tri->indices[2]]);
	}
	for (int i = 0; i < allTris.size(); ++i) {
		NavTri* tri = &allTris[i];
		for (int j = 0; j < 3; ++j) {
			int index = 0;
			file >> index;
			if (index != -1) {
				tri->neighbours[j] = &allTris[index];
			}
		}
	}
}

NavigationMesh::~NavigationMesh()
{
}

bool NavigationMesh::FindPath(const Vector3& from, const Vector3& to, NavigationPath& outPath) {
    const NavTri* startTri = GetTriForPosition(from);
    const NavTri* endTri = GetTriForPosition(to);

    if (!startTri || !endTri) {
        return false; // No valid triangles for the start or end positions
    }

    struct Node {
        const NavTri* tri;
        float gCost; // Cost from start to this node
        float hCost; // Heuristic cost to the end
        float fCost; // Total cost (gCost + hCost)
        const Node* parent;

        Node(const NavTri* t, float g, float h, const Node* p)
            : tri(t), gCost(g), hCost(h), fCost(g + h), parent(p) {}

        bool operator<(const Node& other) const {
            return fCost > other.fCost; // Reverse for priority queue
        }
    };

    auto Heuristic = [](const Vector3& a, const Vector3& b) -> float {
		return Vector::Length(a - b);
        };

    std::priority_queue<Node> openSet;
    std::unordered_map<const NavTri*, float> gScore;
    std::unordered_map<const NavTri*, const Node*> nodeMap;

    Node startNode(startTri, 0.0f, Heuristic(startTri->centroid, endTri->centroid), nullptr);
    openSet.push(startNode);
    gScore[startTri] = 0.0f;
    nodeMap[startTri] = &startNode;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.tri == endTri) {
            // Reconstruct the path
            const Node* pathNode = &current;
            std::vector<Vector3> waypoints;

            while (pathNode != nullptr) {
                waypoints.emplace_back(pathNode->tri->centroid);
                pathNode = pathNode->parent;
            }

            std::reverse(waypoints.begin(), waypoints.end());

            for (const Vector3& waypoint : waypoints) {
                outPath.PushWaypoint(waypoint);
            }

            return true;
        }

        for (int i = 0; i < 3; ++i) {
            const NavTri* neighbor = current.tri->neighbours[i];

            if (!neighbor) {
                continue;
            }

            float tentativeGScore = gScore[current.tri] + Heuristic(current.tri->centroid, neighbor->centroid);

            if (gScore.find(neighbor) == gScore.end() || tentativeGScore < gScore[neighbor]) {
                gScore[neighbor] = tentativeGScore;
                float hCost = Heuristic(neighbor->centroid, endTri->centroid);
                Node neighborNode(neighbor, tentativeGScore, hCost, nodeMap[current.tri]);

                openSet.push(neighborNode);
                nodeMap[neighbor] = &neighborNode;
            }
        }
    }

    return false; // No path found
}

/*
If you have triangles on top of triangles in a full 3D environment, you'll need to change this slightly,
as it is currently ignoring height. You might find tri/plane raycasting is handy.
*/

const NavigationMesh::NavTri* NavigationMesh::GetTriForPosition(const Vector3& pos) const {
	for (const NavTri& t : allTris) {
		Vector3 planePoint = t.triPlane.ProjectPointOntoPlane(pos);

		float ta = Maths::AreaofTri3D(allVerts[t.indices[0]], allVerts[t.indices[1]], planePoint);
		float tb = Maths::AreaofTri3D(allVerts[t.indices[1]], allVerts[t.indices[2]], planePoint);
		float tc = Maths::AreaofTri3D(allVerts[t.indices[2]], allVerts[t.indices[0]], planePoint);

		float areaSum = ta + tb + tc;

		if (abs(areaSum - t.area)  > 0.001f) { //floating points are annoying! Are we more or less inside the triangle?
			continue;
		}
		return &t;
	}
	return nullptr;
}