# -*- coding: utf-8 -*-
"""OMER 购物服务 - 工程示意图统一绘制脚本 (航天未来科技组)"""
import matplotlib
matplotlib.use("Agg")
import matplotlib.font_manager as fm
import glob as _g
for _f in _g.glob("/usr/share/fonts/opentype/noto/NotoSansCJK*.ttc"):
    try:
        fm.fontManager.addfont(_f)
    except Exception:
        pass
import matplotlib.pyplot as plt
import matplotlib.patches as mp
from matplotlib.patches import FancyArrowPatch, Rectangle, Circle, FancyBboxPatch, Polygon, Wedge, Arc
import numpy as np
import os

OUT = "/root/.codebuddy/artifact/2f8a7d30-9c21-4f0b-1d5c-af8dd20f3000/assets"
os.makedirs(OUT, exist_ok=True)

plt.rcParams["font.family"] = "sans-serif"
plt.rcParams["font.sans-serif"] = ["Noto Sans CJK SC", "DejaVu Sans"]
plt.rcParams["axes.unicode_minus"] = False

# 统一色板: 深空航天风
BG = "#0B1B33"      # 深空蓝
BG2 = "#12244A"
PANEL = "#0E2140"
CYAN = "#38E1FF"    # 主强调 青
ORANGE = "#FF9F45"  # 次强调 橙
GOLD = "#FFD166"
WHITE = "#EAF2FF"
GREY = "#8FA6C9"
LINE = "#2E4A78"
GREEN = "#7CFF9B"
RED = "#FF6B6B"

FT = 0.3048  # ft -> m


def newfig(w=10, h=6.4, title=""):
    fig, ax = plt.subplots(figsize=(w, h))
    fig.patch.set_facecolor(BG)
    ax.set_facecolor(BG)
    ax.set_xticks([]); ax.set_yticks([])
    for s in ax.spines.values():
        s.set_visible(False)
    if title:
        ax.set_title(title, color=WHITE, fontsize=15, fontweight="bold", pad=12)
    return fig, ax


def save(fig, name):
    p = os.path.join(OUT, name)
    fig.savefig(p, dpi=170, facecolor=BG, bbox_inches="tight")
    plt.close(fig)
    print("saved", p)


def txt(ax, x, y, s, c=WHITE, fs=9, ha="center", va="center", w="normal", rot=0):
    ax.text(x, y, s, color=c, fontsize=fs, ha=ha, va=va, fontweight=w, rotation=rot)


# 平滑路径辅助
def smooth_path(pts, n=260):
    import numpy as _np
    p = _np.array(pts, dtype=float)
    t = _np.linspace(0, 1, 200)
    from scipy.interpolate import make_interp_spline
    try:
        spl = make_interp_spline(_np.linspace(0, 1, len(p)), p, k=3, axis=0)
        q = spl(t)
    except Exception:
        q = p
    return q[:, 0], q[:, 1]


# ============================================================
# 图1 场地布局与行驶路线 (单位: 英尺, 场地 24 x 20)
# ============================================================
def fig1():
    fig, ax = newfig(11, 8.2, "图1  场地布局与行驶路线（图A 单位：英尺，不按比例）")
    W, D = 24, 20
    # 场地边界
    ax.add_patch(Rectangle((0, 0), W, D, fill=False, ec=GREY, lw=2, ls="--"))
    # 起始线 y=3, x 8..16
    ax.plot([8, 16], [3, 3], color=ORANGE, lw=5, solid_capstyle="butt")
    txt(ax, 12, 2.55, "起始线 8′", ORANGE, 10, w="bold")
    txt(ax, 12, 3.6, "车辆出发区（起始线后）", GREY, 8)
    # 购物区域底色: 起始线以下除两区
    ax.add_patch(Rectangle((0, 3), W, D - 3, fc=BG2, ec="none", alpha=0.55, zorder=0))
    txt(ax, 6.5, 6.4, "购物区域 Shopping Area", CYAN, 11, w="bold", rot=0)
    txt(ax, 5.0, 7.6, "（起始线以下全部空间，\n不含结账区与购物车区域）", GREY, 7.5)
    # 结账区 x2..7 y9..13 ; 购物车区域 x17..22 y9..13
    ax.add_patch(Rectangle((2, 9), 5, 4, fc="#1E3A6E", ec=GOLD, lw=2.5))
    txt(ax, 4.5, 11.9, "结账区", GOLD, 11, w="bold")
    txt(ax, 4.5, 11.0, "Checkout\n5′ × 4′", WHITE, 8)
    ax.add_patch(Rectangle((17, 9), 5, 4, fc="#1E3A6E", ec=GREEN, lw=2.5))
    txt(ax, 19.5, 11.9, "购物车区域", GREEN, 11, w="bold")
    txt(ax, 19.5, 11.0, "Cart Area\n5′ × 4′", WHITE, 8)
    # 尺寸标注
    ax.annotate("", xy=(2, 8.6), xytext=(17, 8.6), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1.2))
    txt(ax, 9.5, 8.15, "10′", GREY, 9, w="bold")
    ax.annotate("", xy=(23.2, 3), xytext=(23.2, 9), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1.2))
    txt(ax, 23.9, 6, "6′", GREY, 9, w="bold", rot=90)
    ax.annotate("", xy=(8, 1.2), xytext=(16, 1.2), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1.2))
    txt(ax, 12, 0.7, "8′", GREY, 9, w="bold")
    # 观众席
    ax.add_patch(Rectangle((0, -2.2), W, 1.6, fc="#182B50", ec=GREY, lw=1, ls=":"))
    txt(ax, 12, -1.4, "观众推荐坐席区域", GREY, 9)
    txt(ax, 12, 19.5, "场地 24′ × 20′（约 7.3m × 6.1m）", WHITE, 9, w="bold")

    # 行驶路线: 航点
    pts = [(12, 1.6), (12, 6.0), (15.5, 8.2), (19.5, 10.0),   # 取购物车
           (15.0, 7.0), (11.0, 6.0),                            # 回购物区取货
           (6.5, 8.5), (4.5, 10.6),                             # 去结账
           (9.0, 7.2), (16.0, 6.8), (19.5, 6.0),                # 结账后继续前进→右移
           (19.5, 0.9),                                         # 上穿起始线平面(购物车朝上穿)
           (19.5, 5.2),                                         # 掉头向下, 购物车朝下穿线→释放
           (16.0, 2.0), (12.0, 1.6)]                            # 空载回起始线后
    # 平滑路线
    xs, ys = smooth_path(pts, n=260)
    # 分段着色
    labels = ["① 取购物车", "② 回购物区取货", "③ 驶向结账区", "④ 结账后前进", "⑤ 释放回位"]
    segs = [(0, 4), (4, 6), (6, 8), (8, 11), (11, 14)]
    colors = [CYAN, CYAN, CYAN, ORANGE, GOLD]
    for (a, b), c in zip(segs, colors):
        ax.plot(xs[int(len(xs)*a/14):int(len(xs)*b/14)+1], ys[int(len(ys)*a/14):int(len(ys)*b/14)+1],
                color=c, lw=3, solid_capstyle="round", zorder=5)
    # 关键点
    key = {0: ("P0 出发/接清单", WHITE), 3: ("P1 挂钩购物车", GREEN),
           5: ("P2 取货×3", CYAN), 7: ("P3 结账卸货", GOLD),
           10: ("P4 上穿起始线平面", ORANGE), 11: ("P5 掉头下穿\n→释放购物车", RED)}
    for i, (s, c) in key.items():
        ax.plot(pts[i][0], pts[i][1], "o", color=c, ms=9, mec=WHITE, mew=1.2, zorder=8)
        dx, dy = -1.8, 1.1
        if i in (0, 10, 11):
            dx, dy = 2.0, 0.6
        if i == 3:
            dx, dy = 2.0, 0.8
        if i == 5:
            dx, dy = -2.4, 0.9
        if i == 7:
            dx, dy = -2.6, 1.3
        txt(ax, pts[i][0] + dx, pts[i][1] + dy, s, c, 8.5, w="bold", ha="center")
    # 购物车自走回位箭头
    ax.annotate("", xy=(19.5, 9.6), xytext=(19.5, 4.6),
                arrowprops=dict(arrowstyle="-|>", color=GREEN, lw=3, ls="--"))
    txt(ax, 21.6, 7.0, "购物车\n自行回位\n(约6′)", GREEN, 8.5, w="bold")
    # 图例
    handles = [plt.Line2D([0], [0], color=CYAN, lw=3), plt.Line2D([0], [0], color=ORANGE, lw=3),
               plt.Line2D([0], [0], color=GOLD, lw=3), plt.Line2D([0], [0], color=GREEN, lw=3, ls="--")]
    ax.legend(handles, ["购物行程(车辆+购物车)", "结账后继续前进", "空载返回起始线后", "购物车释放后自行行进"],
              loc="lower right", facecolor=PANEL, edgecolor=LINE, labelcolor=WHITE, fontsize=8.5,
              bbox_to_anchor=(1.0, 0.02))
    ax.set_xlim(-1.5, 25.5); ax.set_ylim(-2.6, 20.6)
    save(fig, "fig1_field.png")


# ============================================================
# 图2 车辆总装三视图
# ============================================================
def fig2():
    fig, ax = newfig(11, 7.2, "图2  “星驰一号”车辆总布置（俯视 + 侧视，单位：mm）")
    # ---- 俯视图 (左半) ----
    ax.text(2.6, 7.3, "俯视图 TOP", color=CYAN, fontsize=11, fontweight="bold")
    # 底盘 600 x 420
    ax.add_patch(FancyBboxPatch((0.3, 4.6), 4.6, 3.2, boxstyle="round,pad=0.06",
                                fc="#16305C", ec=CYAN, lw=2))
    # 驱动轮 左右
    for wx in (0.62, 4.18):
        ax.add_patch(Rectangle((wx, 5.85), 0.5, 0.9, fc="#0A1526", ec=ORANGE, lw=2))
    txt(ax, 2.6, 6.62, "取货铲/料斗 (前置)", WHITE, 8)
    ax.add_patch(Rectangle((0.55, 6.35), 4.1, 0.5, fc="#22447F", ec=GREY, lw=1))
    # 座椅
    ax.add_patch(FancyBboxPatch((1.5, 4.85), 2.2, 1.1, boxstyle="round,pad=0.05", fc="#1B3B70", ec=GOLD, lw=1.8))
    txt(ax, 2.6, 5.4, "驾驶座 + 脚踏板", GOLD, 8)
    # 拖钩
    ax.add_patch(Rectangle((2.42, 4.35), 0.36, 0.3, fc=ORANGE, ec=WHITE, lw=1.2))
    txt(ax, 2.6, 4.1, "释放拖钩(车尾)", ORANGE, 8)
    txt(ax, 0.72, 6.3, "驱动轮\n?80", ORANGE, 7.5)
    ax.annotate("", xy=(4.9, 7.9), xytext=(4.9, 4.6), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1))
    txt(ax, 5.15, 6.2, "600", GREY, 8, rot=90)
    ax.annotate("", xy=(0.3, 4.35), xytext=(4.9, 4.35), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1))
    txt(ax, 2.6, 4.0, "", GREY, 7)

    # ---- 侧视图 (右半) ----
    ax.text(7.4, 7.3, "侧视图 SIDE", color=CYAN, fontsize=11, fontweight="bold")
    ax.add_patch(FancyBboxPatch((6.1, 3.4), 4.6, 1.5, boxstyle="round,pad=0.06", fc="#16305C", ec=CYAN, lw=2))
    # 驱动轮
    ax.add_patch(Circle((8.0, 3.3), 0.52, fc="#0A1526", ec=ORANGE, lw=2.4))
    ax.add_patch(Circle((8.0, 3.3), 0.14, fc=ORANGE))
    ax.add_patch(Circle((9.3, 3.3), 0.52, fc="#0A1526", ec=ORANGE, lw=2.4))
    ax.add_patch(Circle((9.3, 3.3), 0.14, fc=ORANGE))
    # 万向轮
    ax.add_patch(Circle((6.7, 3.25), 0.24, fc="#0A1526", ec=GREY, lw=1.6))
    ax.add_patch(Circle((10.1, 3.25), 0.24, fc="#0A1526", ec=GREY, lw=1.6))
    # 座椅+人
    ax.add_patch(Rectangle((8.6, 4.9), 1.5, 0.12, fc=GOLD))
    ax.add_patch(FancyBboxPatch((8.9, 5.0), 0.9, 1.35, boxstyle="round,pad=0.04", fc="#2A4E8F", ec=GOLD, lw=1.5))
    ax.add_patch(Circle((9.35, 6.55), 0.33, fc="#F2D3A7", ec=GOLD, lw=1.5))
    txt(ax, 9.35, 6.55, "驾", "#3B2B12", 7.5, w="bold")
    ax.add_patch(Rectangle((8.75, 4.35), 1.1, 0.55, fc="#1B3B70", ec=GREY, lw=1))
    txt(ax, 9.3, 4.6, "脚蹬\n(离地)", GOLD, 6.5)
    # 料斗+铲
    ax.add_patch(Rectangle((6.35, 4.35), 1.1, 0.75, fc="#22447F", ec=CYAN, lw=1.4))
    txt(ax, 6.9, 4.72, "料斗", CYAN, 7.5)
    ax.add_patch(Polygon([(6.1, 4.35), (6.1, 3.75), (6.7, 3.9)], fc=CYAN, ec=WHITE, lw=1))
    txt(ax, 6.2, 3.55, "取货铲", CYAN, 7.5)
    # 电池/电控
    ax.add_patch(Rectangle((7.55, 4.5), 0.8, 0.55, fc="#0F3B2E", ec=GREEN, lw=1.4))
    txt(ax, 7.95, 4.78, "电池", GREEN, 7)
    ax.add_patch(Rectangle((7.55, 3.62), 0.8, 0.5, fc="#3B2B12", ec=GOLD, lw=1.2))
    txt(ax, 7.95, 3.87, "电控盒", GOLD, 6.5)
    # OLED
    ax.add_patch(Rectangle((10.35, 5.1), 0.5, 0.35, fc="#03111E", ec=CYAN, lw=1.4))
    txt(ax, 10.6, 5.62, "OLED\n清单屏", CYAN, 6.5)
    # 拖钩
    ax.add_patch(Rectangle((10.55, 3.9), 0.35, 0.3, fc=ORANGE, ec=WHITE, lw=1.2))
    txt(ax, 10.72, 4.28, "拖钩", ORANGE, 7)
    ax.annotate("", xy=(6.1, 2.8), xytext=(10.7, 2.8), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1))
    txt(ax, 8.4, 2.5, "车长约 900（含铲与拖钩）", GREY, 8)
    ax.annotate("", xy=(6.1, 7.6), xytext=(6.1, 3.4), arrowprops=dict(arrowstyle="<->", color=GREY, lw=1))
    txt(ax, 5.85, 5.5, "总高约 700", GREY, 8, rot=90)
    txt(ax, 8.4, 8.35, "差速驱动：双电机同轴对置 + 前后万向轮，原地转向半径 0", WHITE, 9.5, w="bold")
    ax.set_xlim(0, 11.6); ax.set_ylim(2.0, 8.8)
    save(fig, "fig2_vehicle.png")


# ============================================================
# 图3 电气控制框图
# ============================================================
def fig3():
    fig, ax = newfig(11, 6.6, "图3  车辆电气与控制系统框图")
    def box(x, y, w, h, s, c=CYAN, fc="#16305C", fs=8.5):
        ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.05", fc=fc, ec=c, lw=1.8))
        txt(ax, x + w / 2, y + h / 2, s, WHITE, fs)
        return (x + w / 2, y, x + w / 2, y + h, x, y + h / 2, x + w, y + h / 2)
    def arrow(p1, p2, c=GREY, style="-|>", lw=1.6, ls="-"):
        ax.annotate("", xy=p2, xytext=p1, arrowprops=dict(arrowstyle=style, color=c, lw=lw, linestyle=ls))
    # 电源
    b = box(0.3, 4.6, 1.9, 1.0, "2S 锂电池组\n7.4V 2600mAh", GREEN, "#0F3B2E")
    # 主控
    b2 = box(3.0, 4.6, 1.9, 1.0, "Arduino Nano\n主控 (ATmega328)", GOLD, "#3B2B12")
    # 遥控
    b3 = box(0.3, 2.9, 1.9, 0.9, "遥控手柄\n摇杆×2 + 按键", CYAN)
    b3r = box(0.3, 1.5, 1.9, 0.9, "nRF24L01\n2.4G 接收", CYAN)
    # 驱动
    b4 = box(3.0, 2.9, 1.9, 0.9, "TB6612FNG\n双路 H 桥", ORANGE, "#3B2B12")
    # 电机
    m1 = box(6.0, 3.6, 2.0, 0.9, "左减速电机\n带编码器", ORANGE)
    m2 = box(6.0, 2.4, 2.0, 0.9, "右减速电机\n带编码器", ORANGE)
    # 传感器
    s1 = box(3.0, 1.5, 1.9, 0.9, "MPU6050\n陀螺仪", CYAN)
    # 执行
    e1 = box(6.0, 1.2, 2.0, 0.8, "取货铲舵机", GOLD)
    e2 = box(6.0, 0.2, 2.0, 0.8, "拖钩释放舵机", RED)
    # OLED
    o1 = box(9.0, 2.4, 1.9, 0.9, "OLED 屏\n清单/状态", CYAN)
    # 里程计标注
    txt(ax, 7.0, 5.2, "编码器脉冲 → 里程计 (x, y, θ)", WHITE, 9, w="bold")
    ax.add_patch(FancyBboxPatch((5.7, 4.9), 5.4, 1.3, boxstyle="round,pad=0.05", fc="none", ec=GREY, lw=1.2, ls=":"))
    # 连线
    arrow((2.2, 5.1), (3.0, 5.1), GREEN)
    txt(ax, 2.6, 5.35, "7.4V", GREEN, 7)
    arrow((1.25, 2.9), (1.25, 2.4), CYAN)
    arrow((2.2, 1.95), (3.0, 1.95), CYAN)
    txt(ax, 2.6, 2.2, "SPI", CYAN, 7)
    arrow((2.2, 3.35), (3.0, 3.35), CYAN)
    txt(ax, 2.6, 3.6, "指令", CYAN, 7)
    arrow((4.9, 3.35), (6.0, 3.35), ORANGE)
    txt(ax, 5.45, 3.6, "PWM", ORANGE, 7)
    arrow((4.0, 3.9), (4.0, 4.6), GOLD)
    txt(ax, 4.35, 4.25, "I2C/控制", GREY, 6.5, rot=90)
    arrow((4.9, 5.1), (5.7, 5.1), GREY, ls=":")
    arrow((4.9, 1.95), (4.45, 1.5) if False else (4.0, 2.4), GREY, ls=":")
    arrow((6.0, 1.6), (4.9, 1.95), GOLD, ls="-")
    arrow((6.0, 0.6), (4.6, 1.2), RED)
    arrow((8.0, 4.05), (8.0, 4.9), GREY, style="<|-", ls=":")
    arrow((6.0, 2.85), (4.9, 3.1), GREY, style="<|-", ls=":")
    txt(ax, 5.4, 2.95, "AB相", GREY, 6.5)
    arrow((4.9, 4.85), (9.0, 2.9), CYAN, lw=1.2, ls=":")
    # 模式说明
    ax.add_patch(FancyBboxPatch((0.3, 0.15), 5.0, 0.85, boxstyle="round,pad=0.05", fc=PANEL, ec=LINE, lw=1.2))
    txt(ax, 2.8, 0.75, "双模控制：", WHITE, 8.5, w="bold")
    txt(ax, 2.8, 0.42, "AUTO 航点自动巡航（里程计+陀螺仪闭环）/ MANUAL 手动随时接管", GREY, 8)
    ax.set_xlim(0, 11.4); ax.set_ylim(0, 6.2)
    save(fig, "fig3_elec.png")


# ============================================================
# 图4 购物车 + 挂钩/释放/回程机构
# ============================================================
def fig4():
    fig, ax = newfig(11, 6.8, "图4  “回巢号”购物车：连接、释放与自行回位机构")
    # ---- 左: 购物车侧视 ----
    ax.text(2.7, 6.3, "购物车侧视（PVC 管框 + 帆布篮）", color=CYAN, fontsize=10, fontweight="bold")
    ax.add_patch(FancyBboxPatch((0.6, 3.0), 3.6, 2.2, boxstyle="round,pad=0.04", fc="#16305C", ec=CYAN, lw=2))
    ax.add_patch(Rectangle((0.9, 3.4), 3.0, 1.5, fc="#22447F", ec=GREY, lw=1))
    txt(ax, 2.4, 4.15, "帆布货篮\n(3件商品)", WHITE, 8)
    # 轮
    ax.add_patch(Circle((1.3, 2.75), 0.28, fc="#0A1526", ec=ORANGE, lw=2))
    ax.add_patch(Circle((3.5, 2.75), 0.28, fc="#0A1526", ec=ORANGE, lw=2))
    txt(ax, 1.3, 2.2, "后固定轮", ORANGE, 7)
    txt(ax, 3.5, 2.2, "后固定轮", ORANGE, 7)
    # 前轮(驱动)
    ax.add_patch(Circle((1.1, 2.75), 0.0, fc="none"))
    ax.add_patch(Circle((4.0, 2.75), 0.0, fc="none"))
    # 拖杆
    ax.add_patch(Rectangle((4.2, 3.3), 1.6, 0.28, fc="#0A1526", ec=RED, lw=1.8))
    txt(ax, 5.0, 3.9, "拖杆 + T 型钩头", RED, 8, w="bold")
    # 回程模块
    ax.add_patch(FancyBboxPatch((1.6, 2.95), 1.5, 0.6, boxstyle="round,pad=0.04", fc="#0F3B2E", ec=GREEN, lw=1.8))
    txt(ax, 2.35, 3.25, "回程驱动模块", GREEN, 7.5, w="bold")
    ax.annotate("", xy=(3.35, 2.75), xytext=(3.1, 3.1), arrowprops=dict(arrowstyle="-|>", color=GREEN, lw=1.5))
    txt(ax, 4.6, 2.6, "驱动右前轮\n(电磁离合)", GREEN, 7.5)
    # 前轮标记
    ax.add_patch(Circle((0.95, 2.75), 0.0, fc="none"))
    for x, lab in ((0.95, "前万向轮"), (4.35, "前驱动轮")):
        pass
    ax.add_patch(Circle((4.55, 2.75), 0.28, fc="#0A1526", ec=GREEN, lw=2.4))
    txt(ax, 4.55, 2.2, "前驱动轮", GREEN, 7)

    # ---- 右: 释放时序 ----
    ax.text(8.4, 6.3, "释放时序（四个状态）", color=CYAN, fontsize=10, fontweight="bold")
    steps = [("S1 拖行\n离合分离\n电机空转", "#16305C", CYAN),
             ("S2 穿线瞬间\n舵机转爪 90°\nT 钩脱开", "#3B2B12", GOLD),
             ("S3 离合吸合\n电机驱动\n走 1.8 m", "#0F3B2E", GREEN),
             ("S4 行程到\n电机断电\n弹性阻尼停", "#3B2B12", ORANGE)]
    for i, (s, fc, ec) in enumerate(steps):
        y = 5.3 - i * 1.25
        ax.add_patch(FancyBboxPatch((6.6, y), 3.6, 0.95, boxstyle="round,pad=0.05", fc=fc, ec=ec, lw=1.8))
        txt(ax, 8.4, y + 0.48, s, WHITE, 8)
        if i < 3:
            ax.annotate("", xy=(8.4, y - 0.28), xytext=(8.4, y), arrowprops=dict(arrowstyle="-|>", color=GREY, lw=1.5))
    # 底部说明
    ax.add_patch(FancyBboxPatch((0.5, 0.15), 10.2, 0.75, boxstyle="round,pad=0.05", fc=PANEL, ec=LINE, lw=1.2))
    txt(ax, 5.6, 0.65, "行程可调：编码器测距 1.2 ~ 2.2 m 可编程；速度 0.4 m/s；末端毛毡阻尼垫 + 海绵保险杠，保证停在 5′×4′ 区域内",
        WHITE, 8.5)
    txt(ax, 5.6, 0.38, "拖行阶段由车辆推进（电机分离），符合“随车辆行进并由车辆推进”；释放后“自行行进至购物车区域”", GREY, 7.5)
    ax.set_xlim(0, 11.2); ax.set_ylim(0, 6.8)
    save(fig, "fig4_cart.png")


# ============================================================
# 图5 卸货装置
# ============================================================
def fig5():
    fig, ax = newfig(11, 6.4, "图5  “星臂”卸货装置：三工位回转卸料（独立于车辆）")
    # 俯视: 结账区布局
    ax.text(3.1, 6.0, "俯视：结账区工作循环", color=CYAN, fontsize=10, fontweight="bold")
    # 结账区
    ax.add_patch(Rectangle((0.4, 1.6), 4.6, 3.6, fc="#12244A", ec=GOLD, lw=2))
    txt(ax, 2.7, 1.95, "结账区 Checkout（5′×4′）", GOLD, 8.5, w="bold")
    # 立柱
    ax.add_patch(Circle((2.7, 4.4), 0.35, fc="#3B2B12", ec=ORANGE, lw=2.4))
    txt(ax, 2.7, 4.4, "柱", WHITE, 7.5, w="bold")
    # 三工位
    stations = [(2.7, 5.35, "取料位\n(购物车侧)", GREEN), (4.3, 4.4, "过渡位", GREY), (2.7, 3.45, "放料位\n(结账台)", GOLD)]
    for x, y, s, c in stations:
        ax.add_patch(Circle((x, y), 0.42, fc=PANEL, ec=c, lw=2))
        txt(ax, x, y, "", WHITE, 6.5)
        dy = 0.75 if y > 4.4 else (-0.75 if y < 4.4 else 0)
        dx = 0.95 if abs(y - 4.4) < 0.1 else 0
        txt(ax, x + (dx if x > 2.7 else -0), y + dy, s, c, 7.5, w="bold")
    # 臂
    ax.plot([2.7, 2.7, 4.3], [4.4, 5.35, 4.4], color=ORANGE, lw=3.5, solid_capstyle="round")
    ax.plot([2.7, 2.7], [4.4, 3.45], color=ORANGE, lw=3.5, solid_capstyle="round", alpha=0.4)
    # 旋转箭头
    th = np.linspace(0.5, 5.7, 60)
    ax.plot(2.7 + 0.95 * np.cos(th), 4.4 + 0.95 * np.sin(th), color=CYAN, lw=1.6, ls="--")
    ax.annotate("", xy=(2.7 + 0.95 * np.cos(0.55), 4.4 + 0.95 * np.sin(0.55)),
                xytext=(2.7 + 0.95 * np.cos(0.75), 4.4 + 0.95 * np.sin(0.75)),
                arrowprops=dict(arrowstyle="-|>", color=CYAN, lw=1.6))
    # 购物车/结账台
    ax.add_patch(Rectangle((1.4, 4.95), 2.6, 0.9, fc="#16305C", ec=GREEN, lw=2))
    txt(ax, 2.7, 5.4, "购物车停靠位", GREEN, 7.5, w="bold")
    ax.add_patch(Rectangle((1.4, 2.55), 2.6, 0.75, fc="#3B2B12", ec=GOLD, lw=2))
    txt(ax, 2.7, 2.92, "结账台（垫高 300mm）", GOLD, 7.5, w="bold")
    # 侧视
    ax.text(8.3, 6.0, "侧视：回转臂结构", color=CYAN, fontsize=10, fontweight="bold")
    ax.add_patch(Rectangle((6.6, 1.4), 3.4, 0.35, fc="#12244A", ec=GOLD, lw=1.6))
    txt(ax, 8.3, 1.2, "配重底座（三脚星形，独立不连车）", GOLD, 7.5)
    ax.add_patch(Rectangle((8.05, 1.75), 0.5, 2.4, fc="#3B2B12", ec=ORANGE, lw=1.8))
    txt(ax, 8.3, 4.35, "蜗轮蜗杆减速电机\n12V 1:100", ORANGE, 7.5, w="bold")
    # 臂 + 托盘
    ax.plot([8.3, 10.1], [4.15, 4.5], color=ORANGE, lw=4, solid_capstyle="round")
    ax.add_patch(Rectangle((10.05, 4.5), 0.55, 0.18, fc=CYAN))
    txt(ax, 10.3, 4.85, "伺服翻料托盘\n(V型弹性夹)", CYAN, 7)
    # 商品
    ax.add_patch(Rectangle((10.15, 4.68), 0.34, 0.5, fc=GOLD))
    txt(ax, 10.32, 5.5, "商品", GOLD, 7)
    ax.annotate("", xy=(10.1, 3.0), xytext=(10.3, 4.65), arrowprops=dict(arrowstyle="-|>", color=GREY, lw=1.4, ls="--"))
    txt(ax, 9.6, 3.3, "翻转倒料", GREY, 7)
    # 手持控制盒
    ax.add_patch(FancyBboxPatch((6.6, 4.6), 1.3, 0.85, boxstyle="round,pad=0.05", fc=PANEL, ec=CYAN, lw=1.8))
    txt(ax, 7.25, 5.02, "手持按钮盒\n(操作员)", CYAN, 7.5)
    ax.annotate("", xy=(8.15, 4.15), xytext=(7.9, 4.6), arrowprops=dict(arrowstyle="-|>", color=CYAN, lw=1.4))
    txt(ax, 5.6, 0.35, "三件商品循环 3 次：托盘 V 型夹逐件夹取 → 回转 120°×3 → 翻转落入结账台；全程人力仅按按钮（D6b 创造性加分项）",
        WHITE, 8)
    ax.set_xlim(0, 11.2); ax.set_ylim(0, 6.4)
    save(fig, "fig5_unload.png")


# ============================================================
# 图6 8分钟时间轴
# ============================================================
def fig6():
    fig, ax = newfig(11, 6.6, "图6  8 分钟比赛过程时间轴（含 60 秒安全余量）")
    tasks = [
        ("进场布置（1 人摆道具/2 人展道具）", 0, 40, GREY),
        ("开场白 + 音乐起 + 主题亮相", 40, 55, CYAN),
        ("P0 购物助理递交购物清单（HUD 屏同步点亮）", 55, 80, CYAN),
        ("P1 车辆越线 → 购物车区域自动挂钩", 80, 125, GREEN),
        ("P2 回购物区域，取货 3 件（牛奶缺货→豆奶替代，助理批准）", 125, 215, CYAN),
        ("P3 驶向结账区", 215, 250, ORANGE),
        ("P4 结账：星臂卸货 3 件 + 助理核对订单", 250, 360, GOLD),
        ("P5 继续前进→上穿起始线平面→掉头", 360, 415, ORANGE),
        ("P6 购物车下穿线释放 → 自行回位停车区", 415, 455, GREEN),
        ("P7 车辆空载返回起始线后停稳", 455, 490, GOLD),
        ("结尾歌舞 + 谢幕 + 定格", 490, 540, CYAN),
        ("安全余量（处理意外/补试）", 540, 600, RED),
    ]
    y = 5.6
    for s, a, b, c in tasks:
        ax.add_patch(Rectangle((0.9 + a * 0.0135, y), (b - a) * 0.0135, 0.55, fc=c, ec="none", alpha=0.9))
        txt(ax, 0.85, y + 0.27, "", WHITE, 7, ha="right")
        y -= 0.62
    # 分钟刻度
    for m in range(0, 11):
        x = 0.9 + m * 60 * 0.0135
        ax.plot([x, x], [0.6, 5.9], color=LINE, lw=0.6, alpha=0.6)
        if m <= 10:
            txt(ax, x, 0.3, f"{m}′" if m < 10 else "10′", GREY, 8)
    # 任务标签放右侧
    y = 5.6
    for s, a, b, c in tasks:
        mm, ss = divmod(a, 60)
        txt(ax, 9.55, y + 0.27, s, WHITE, 7.6, ha="left")
        txt(ax, 0.88, y + 0.27, f"{mm:02d}:{ss:02d}", GREY, 7.5, ha="right")
        y -= 0.62
    ax.set_xlim(0, 18.5); ax.set_ylim(0, 6.3)
    save(fig, "fig6_timeline.png")


# ============================================================
# 图7 成本构成饼图
# ============================================================
def fig7():
    fig, ax = newfig(9.5, 5.6, "图7  总成本 ＄160 构成（上限 ＄165，余量 ＄5）")
    items = [("车辆机械", 33), ("驱动与电控", 50), ("电源", 19),
             ("购物车(含回程)", 25), ("星臂卸货装置", 12), ("道具与风格", 20), ("耗材", 1)]
    vals = [v for _, v in items]
    cols = [CYAN, ORANGE, GREEN, GOLD, "#C792EA", "#4FC3F7", GREY]
    wedges, _ = ax.pie(vals, colors=cols, startangle=90, counterclock=False,
                       wedgeprops=dict(width=0.42, edgecolor=BG, lw=2))
    total = sum(vals)
    txt(ax, 0, 0.12, f"＄{total}", WHITE, 22, w="bold")
    txt(ax, 0, -0.16, f"上限 ＄165 · 余量 ＄5", GREEN, 11)
    # 外部图例
    lab = [f"{n}  ＄{v}" for n, v in items]
    ax.legend(wedges, lab, loc="center left", bbox_to_anchor=(1.02, 0.5),
              facecolor=PANEL, edgecolor=LINE, labelcolor=WHITE, fontsize=9)
    save(fig, "fig7_cost.png")


if __name__ == "__main__":
    for f in (fig1, fig2, fig3, fig4, fig5, fig6, fig7):
        try:
            f()
        except Exception as e:
            print("ERR", f.__name__, e)