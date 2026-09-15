/**
 * @class   igQtAnimationWidget
 * @brief   igQtAnimationWidget's brief
 */

#pragma once
#include <ui_Animation.h>
#include <IQCore/igQtExportModule.h>
#include <iGameDataObject.h>
#include <ForceStaticMesh/iGameForceStaticMeshFilter.h>
#include <string>
#include <vector>
class igQtAnimationVcrController;
class IG_QT_MODULE_EXPORT igQtAnimationWidget : public QWidget{

    Q_OBJECT

public:
    igQtAnimationWidget(QWidget* parent = nullptr);

    // 检查动画是否正在播放（用于阻止播放期间重新初始化组件）
    bool IsPlaying() const { return m_IsAnimationPlaying; }

    // 声明「至少需要缓存多少帧」。
    // 逐帧计算出的派生属性（如涡量）只存在于帧对象上，一旦缓存被清空，
    // 下次切帧会从磁盘重新读出不含该属性的新对象，既丢结果又会造成父子属性数不一致。
    // 设置后 initAnimationComponents 不再把缓存重置为 0。
    void setPreferredCacheNum(int n);

    // 开启 / 关闭「播放时按需计算涡量」。
    // 开启后每次切帧都会检查当前帧是否已有 vorticities：
    // 命中缓存（帧对象上已带该属性）则直接复用，否则同步计算完再继续渲染该帧。
    void setVortexAutoCompute(bool enabled, const std::string& sourceAttrName = std::string());
    bool isVortexAutoCompute() const { return m_VortexAutoCompute; }

    // 确保 obj 的「当前帧」已有 vorticities：已存在则直接返回（不产生任何开销，
    // 也不会触碰进度条），不存在才真正计算，并把该属性登记到父容器的
    // AttributeSet（模型树 / 云图靠它寻址）。
    // frameIndexForDisplay 仅用于进度条文字，传 -1 表示不显示帧号。
    // 返回 vorticities 在父容器中的索引；失败返回 -1。
    int ensureVortexForCurrentFrame(iGame::DataObject::Pointer obj, const std::string& sourceAttrName,
                                    int frameIndexForDisplay = -1);

    // ---- 静态网格缓存（Force Static Mesh）的时间步联动 ----
    // 注册一个「静态网格缓存跟随项」：时间步变化（拖动时间轴 / 播放）后，
    // 自动把输入当前帧的属性同步到缓存输出上，几何保持缓存建立时的那一帧。
    void registerStaticMeshCache(iGame::DataObject::Pointer input,
                                 iGame::DataObject::Pointer output,
                                 iGame::ForceStaticMeshFilter::Pointer filter);
    void unregisterStaticMeshCache(iGame::DataObject::Pointer input);
    bool hasStaticMeshCache(iGame::DataObject::Pointer input) const;
    iGame::ForceStaticMeshFilter::Pointer getStaticMeshFilter(iGame::DataObject::Pointer input) const;
    // 按模型查静态网格缓存：obj 可以是输入端模型，也可以是缓存输出模型。
    // 找到时返回 true 并给出输入、输出与过滤器实例（供属性面板等使用）。
    bool getStaticMeshCacheByModel(iGame::DataObject::Pointer obj,
                                   iGame::DataObject::Pointer& input,
                                   iGame::DataObject::Pointer& output,
                                   iGame::ForceStaticMeshFilter::Pointer& filter) const;

public slots:
    void initAnimationComponents();

    bool saveAnimation();

    void ClearAnimationVCRInfo();
private slots:
    void playAnimation_snap(unsigned int keyframe_idx);
    void playAnimation_interpolate(int keyframe_0, float t);
    void btnPlay_finishLoop();
    void updateAnimationComponentsKeyframeSum(int keyframeSum);
    void changeAnimationMode();
    void onCacheNumChanged(int cacheNum);  // 缓存数量变化槽函数


signals:
    void UpdateScene();
    void AnimationFrameChanged();  // Signal when animation frame changes, triggers scalar UI update

    // 静态网格缓存已随新的时间步更新（主窗口据此刷新模型树属性与场景渲染）
    // attributesChanged 为 true 时属性集合发生变化，需要重建模型树子项
    void StaticMeshCacheUpdated(iGame::DataObject::Pointer output, bool attributesChanged,
                                QString message);

    void PlayAnimation_snap(int keyframe_idx);

    void PlayAnimation_interpolate(int keyframe_0, float t);


private:
    // 静态网格缓存跟随项：输入模型 → 缓存输出 + 对应的过滤器实例
    struct StaticMeshCacheBinding {
        iGame::DataObject::Pointer Input;
        iGame::DataObject::Pointer Output;
        iGame::ForceStaticMeshFilter::Pointer Filter;
        std::string AttributeSignature; // 上次同步时的属性签名（判断是否需要重建模型树子项）
    };

    // 遍历已注册的静态网格缓存跟随项，把输入当前帧的属性同步到缓存输出
    void syncStaticMeshCaches(int frameIdx, float timeValue);
    // 刷新缓存输出的渲染数据（属性变化后必须重建，否则云图仍是旧值）
    static void refreshStaticMeshOutput(iGame::DataObject::Pointer output);

    // 若 obj 是某个缓存的输出，返回对应绑定（否则 nullptr）。
    StaticMeshCacheBinding* findStaticMeshBindingByOutput(iGame::DataObject* obj);
    // 取得用于驱动某个模型的帧列表：缓存输出节点用其输入的时间帧
    iGame::StreamingData::Pointer timeFramesForModel(iGame::DataObject::Pointer obj);
    // 缓存输出节点的「时间步更新」：推进输入 → filter 重新执行（几何固定、属性更新）
    bool updateStaticMeshOutputAtTimeStep(iGame::DataObject::Pointer output, int frameIdx);

private:
    Ui::Animation* ui;
    igQtAnimationVcrController* VcrController;
    bool m_IsAnimationPlaying{false}; // 动画播放状态标记
    int m_PreferredCacheNum{0};       // 外部声明的最小缓存帧数，见 setPreferredCacheNum
    bool m_VortexAutoCompute{false};  // 播放时按需补算涡量，见 setVortexAutoCompute
    std::string m_VortexSourceAttr;   // 计算涡量所用的源矢量属性名
    // 上次绑定按需计算的模型；用于「切换模型时自动关闭」，
    // 而在同一模型上选属性（同样会触发 initAnimationComponents）时保持开启
    iGame::DataObject* m_VortexBoundModel{nullptr};

    std::vector<StaticMeshCacheBinding> m_StaticMeshCaches;
};
