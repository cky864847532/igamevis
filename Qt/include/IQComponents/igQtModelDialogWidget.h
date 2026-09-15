#pragma once

#include <iGameSceneManager.h>

#include <IQComponents/igQtModelTreeWidget.h>
#include <IQComponents/igQtPropertyTreeWidget.h>
#include <IQCore/igQtExportModule.h>

#include <ui_layerDialog.h>

#include <Plugin/qtpropertybrowser/qteditorfactory.h>
#include <Plugin/qtpropertybrowser/qttreepropertybrowser.h>
#include <Plugin/qtpropertybrowser/qtvariantproperty.h>
#include <QMouseEvent>
#include <QObject>
#include <QString>
#include <QTreeWidget>
#include <iostream>

class QDockWidget;

class IG_QT_MODULE_EXPORT igQtModelDialogWidget : public QObject {
    Q_OBJECT
public:
    igQtModelDialogWidget(QWidget* parent);
    ~igQtModelDialogWidget() override = default;

    /** 上半部分：圖層/模型樹，可單獨拖出懸浮 */
    QDockWidget* getTreeDock() const { return m_treeDock; }
    /** 下半部分：屬性 / 模型資訊 */
    QDockWidget* getPropertiesDock() const { return m_propertiesDock; }


public slots:
    int addModelToModelTree(iGame::Model::Pointer model);
    ModelTreeWidgetItem* getItemFromObject(iGame::DataObject::Pointer obj);
    void updateAllAttriubute(iGame::DataObject::Pointer obj);
    void updateItemName(iGame::DataObject::Pointer obj);
    int addDataObjectToModelTree(iGame::DataObject::Pointer obj, ItemSource source);
    int updateCurrentModelInfo();
    void updateCurrentModelProperty(iGame::Model* model);
    void updateCurrentModelProperty();
    int updateCloudPicture();
    void deleteCurrentModel();
    void onPropertyChanged(QtProperty* property, const QVariant& value);
    iGame::Model* GetCurrentModel();
    // Force Static Mesh：属性面板中的 “Force Cache Computation” 勾选项（仿 ParaView 的 Properties 面板）。
    // visible=false 时该项置灰不可用（当前模型不是静态网格缓存/输入）。
    void setStaticMeshCacheProperty(bool visible, bool value);
    void setCurrentItem(QTreeWidgetItem* item) {
        if (modelTreeWidget) modelTreeWidget->setCurrentItem(item);
    }
    void positionTreeDockToRendererCorner(QWidget* rendererWidget);
signals:
    void CurrendModelChanged();
    void CloudPictureChanged();
    void ModelDeleted(const std::string& modelName);  // Emitted when model is deleted
    void Update();
    // 用户在属性面板中切换了 “Force Cache Computation”
    void StaticMeshCacheForceComputeChanged(bool value);

private:
    //iGame::Model* currentModel;

    igQtModelTreeWidget* modelTreeWidget;
    QtTreePropertyBrowser* propertyWidget;
    QTabWidget* tabWidget;

    QtVariantPropertyManager* propertyManager;
    QtVariantEditorFactory* editFactory;

    QtProperty* objectGroup;
    QtVariantProperty* prop_PointSize;
    QtVariantProperty* pror_LineWidth;
    QtVariantProperty* prop_Transparency;
    QtVariantProperty* prop_ForceCacheComputation = nullptr; // Force Static Mesh：强制重建缓存
    // 程序性地刷新上面的属性时置位，避免被当成用户勾选
    bool m_UpdatingStaticMeshProperty{false};
    // 该项当前是否显示在对象属性组中（只有 ForceStaticMesh 生成的缓存模型才显示）
    bool m_StaticMeshPropertyShown{false};

    Ui::LayerDialog* ui;
    QDockWidget* m_treeDock = nullptr;       // 上半
    QDockWidget* m_propertiesDock = nullptr; // 下半
    static bool m_AutoAccelerate;
};
