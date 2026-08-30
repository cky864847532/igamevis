#include "Periodic/iGameAngularPeriodicFilter.h"
#include "iGamePoints.h"
#include "iGameSurfaceMesh.h"
#include <cmath>

IGAME_NAMESPACE_BEGIN

AngularPeriodicFilter::AngularPeriodicFilter() {
    SetNumberOfInputs(1);
    SetNumberOfOutputs(1);
}

void AngularPeriodicFilter::SetRotationAxis(const Point& origin, const Vector3d& axis) {
    m_AxisOrigin = origin;
    m_AxisNormalized = axis;
    if (m_AxisNormalized.length() > 1e-12) {
        m_AxisNormalized.normalize();
    }
}

bool AngularPeriodicFilter::Execute() {
    auto input = GetInput(0);
    if (input == nullptr) {
        m_Message = "no input mesh";
        return false;
    }
    if (m_AxisNormalized.length() < 1e-12) {
        m_Message = "rotation axis has zero length";
        return false;
    }
    if (m_NumberOfCopies < 1) {
        m_Message = "number of copies is invalid";
        return false;
    }

    auto mesh = DynamicCast<PointSet>(input);
    if (mesh == nullptr) {
        m_Message = "input is not a surface/unstructured mesh";
        return false;
    }

    auto output = UnstructuredMesh::New();

    float stepAngle = m_Angle / m_NumberOfCopies * 3.14159265358979f / 180.0f;
    for (int i = 0; i < m_NumberOfCopies; ++i) {
        CopyRotated(output.get(), mesh.get(), stepAngle * i);
    }

    if (output->GetNumberOfPoints() == 0) {
        m_Message = "output is empty";
        return false;
    }

    SetOutput(output);
    return true;
}

Point AngularPeriodicFilter::RotatePoint(const Point& p, float angleRad) {
    double cosA = std::cos(angleRad);
    double sinA = std::sin(angleRad);

    Vector3d v(p[0] - m_AxisOrigin[0],
               p[1] - m_AxisOrigin[1],
               p[2] - m_AxisOrigin[2]);
    Vector3d axis = m_AxisNormalized;

    Vector3d cross = axis.cross(v);
    double dot = axis.dot(v);

    Vector3d rotated = v * cosA + cross * sinA + axis * (dot * (1.0 - cosA));

    return Point(static_cast<float>(m_AxisOrigin[0] + rotated[0]),
                 static_cast<float>(m_AxisOrigin[1] + rotated[1]),
                 static_cast<float>(m_AxisOrigin[2] + rotated[2]));
}

void AngularPeriodicFilter::CopyRotated(UnstructuredMesh* out, PointSet* src, float rotationAngleRad) {
    if (out == nullptr || src == nullptr) return;

    IGsize pointOffset = out->GetNumberOfPoints();

    auto points = src->GetPoints();
    IGsize numPoints = src->GetNumberOfPoints();
    for (IGsize i = 0; i < numPoints; ++i) {
        out->AddPoint(RotatePoint(points->GetPoint(i), rotationAngleRad));
    }

    auto cellArray = src->GetCellArray();
    if (cellArray == nullptr) return;

    auto uMesh = DynamicCast<UnstructuredMesh>(src);
    IGsize numCells = cellArray->GetNumberOfCells();
    for (IGsize c = 0; c < numCells; ++c) {
        igIndex ids[16];
        int size = cellArray->GetCellIds(c, ids);
        if (size <= 0 || size > 16) continue;

        std::vector<igIndex> cellIds(size);
        for (int j = 0; j < size; ++j) {
            cellIds[j] = ids[j] + static_cast<igIndex>(pointOffset);
        }

        IGenum cellType = uMesh != nullptr ? uMesh->GetCellType(c)
                                           : SurfaceMesh::GetFaceTypeWithPointNum(size);
        out->AddCell(cellIds.data(), size, cellType);
    }
}

IGAME_NAMESPACE_END
