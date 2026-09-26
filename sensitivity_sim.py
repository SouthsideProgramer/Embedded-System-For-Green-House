import numpy as np

# Greenhouse dimensions
W = 20.0  # meters (x)
L = 50.0  # meters (y)

configs = [
    {"N": 6, "grid": (2, 3), "zones": 2, "nodes_per_zone": 3},
    {"N": 8, "grid": (2, 4), "zones": 2, "nodes_per_zone": 4},
    {"N": 10, "grid": (2, 5), "zones": 2, "nodes_per_zone": 5},
    {"N": 12, "grid": (2, 6), "zones": 3, "nodes_per_zone": 4},
    {"N": 16, "grid": (2, 8), "zones": 4, "nodes_per_zone": 4},
    {"N": 20, "grid": (2, 10), "zones": 4, "nodes_per_zone": 5},
]

# Create fine 2D evaluation grid
nx, ny = 100, 250
x = np.linspace(0, W, nx)
y = np.linspace(0, L, ny)
XX, YY = np.meshgrid(x, y)

# True temperature field under solar + airflow gradient
# y=0 is cool inlet (28C), y=50 is warm exhaust (32C)
# solar effect adds +1.5C near edges (x=0 and x=20)
T_true = 28.0 + 4.0 * (YY / L) + 1.5 * np.cos(np.pi * (XX - W/2) / W)**2

# Localized heat event: fan failure at (x=17, y=45)
# Hot plume spreading over time
event_x, event_y = 17.0, 45.0
plume_radius = 6.0
T_event = 6.0 * np.exp(-((XX - event_x)**2 + (YY - event_y)**2) / (2 * (plume_radius/2)**2))
T_with_event = T_true + T_event

results = []

for cfg in configs:
    N = cfg["N"]
    gx, gy = cfg["grid"]
    
    # Node coordinates
    dx = W / gx
    dy = L / gy
    node_x = np.linspace(dx/2, W - dx/2, gx)
    node_y = np.linspace(dy/2, L - dy/2, gy)
    nX, nY = np.meshgrid(node_x, node_y)
    nodes = np.column_stack([nX.ravel(), nY.ravel()])
    
    # 1. Max distance from any point in greenhouse to nearest node
    d_max = np.sqrt((dx/2)**2 + (dy/2)**2)
    
    # 2. Spatial sampling and IDW reconstruction
    # Sample true field at nodes
    # node values
    node_T = 28.0 + 4.0 * (nodes[:, 1] / L) + 1.5 * np.cos(np.pi * (nodes[:, 0] - W/2) / W)**2
    # IDW reconstruction across fine grid
    grid_pts = np.column_stack([XX.ravel(), YY.ravel()])
    
    # Distances from all grid points to all nodes
    dists = np.linalg.norm(grid_pts[:, np.newaxis, :] - nodes[np.newaxis, :, :], axis=2)
    dists = np.maximum(dists, 1e-4)
    weights = 1.0 / (dists ** 2)
    weights /= weights.sum(axis=1, keepdims=True)
    T_recon = (weights * node_T).sum(axis=1).reshape(XX.shape)
    
    rmse = np.sqrt(np.mean((T_true - T_recon)**2))
    
    # 3. Detection of localized fan failure
    # Distance from event center to nearest node
    dist_to_event = np.min(np.linalg.norm(nodes - np.array([event_x, event_y]), axis=1))
    # Hot plume peak is at event center (dT = 6C). Plume profile dT(d) = 6 * exp(-d^2 / 9).
    # Nearest node sees dT_node = 6 * exp(-dist_to_event^2 / 9).
    # Heat rise rate at plume center is 0.8 C/min. Time for nearest node to see +2.0C rise:
    # dT_node(t) = 0.8 * t * exp(-dist_to_event^2 / (2 * (plume_radius/2)**2))
    # Threshold = 2.0 C rise:
    spatial_attenuation = np.exp(-dist_to_event**2 / (2 * (plume_radius/2)**2))
    if spatial_attenuation > 0.05:
        t_detect = 2.0 / (0.8 * spatial_attenuation)
    else:
        t_detect = 99.9  # missed / out of range
        
    # 4. Fault tolerance: Probability all zones have >= 3 good nodes (with p_fail = 0.05)
    import math
    M = cfg["nodes_per_zone"]
    p_good = 0.95
    p_zone_valid = sum(math.comb(M, k) * (p_good**k) * ((1-p_good)**(M-k)) for k in range(3, M + 1))
    p_all_zones_valid = p_zone_valid ** cfg["zones"]
    
    # 5. Cost estimation (Design B CAPEX: $110 GW + $29.30/node for N indoor + 1 outdoor N00)
    capex_B = 110.0 + (N + 1) * 29.30
    
    # 6. TDMA slot occupancy in 30s frame (each slot 1.5s, N indoor + 1 outdoor N00)
    slot_occupancy = ((N + 1) * 1.5) / 30.0 * 100
    
    results.append({
        "N": N,
        "grid": f"{gx}x{gy}",
        "zones": cfg["zones"],
        "d_max": d_max,
        "rmse": rmse,
        "dist_event": dist_to_event,
        "t_detect": t_detect,
        "p_resilience": p_all_zones_valid * 100,
        "capex_B": capex_B,
        "tdma_duty": slot_occupancy
    })

print(f"{'N':<4} {'Grid':<6} {'Zones':<6} {'d_max(m)':<9} {'RMSE(C)':<8} {'d_event(m)':<11} {'t_det(min)':<11} {'P_avail(%)':<11} {'CAPEX($)':<9} {'TDMA(%)':<8}")
print("-" * 90)
for r in results:
    print(f"{r['N']:<4} {r['grid']:<6} {r['zones']:<6} {r['d_max']:<9.2f} {r['rmse']:<8.3f} {r['dist_event']:<11.2f} {r['t_detect']:<11.2f} {r['p_resilience']:<11.1f} {r['capex_B']:<9.1f} {r['tdma_duty']:<8.1f}")

