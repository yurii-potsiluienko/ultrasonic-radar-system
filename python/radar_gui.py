import serial
import pygame
import math
import sys

# --- CONFIGURATION ---
SERIAL_PORT = 'COM5'
BAUD_RATE = 115200    
WIDTH, HEIGHT = 900, 550
CENTER = (WIDTH // 2, HEIGHT - 80)
MAX_DISTANCE = 100    # cm

# Colors
COLOR_BG = (5, 20, 5)
COLOR_GRID = (0, 80, 0)
COLOR_BEAM = (0, 255, 0)
COLOR_OBJ = (255, 0, 0)
COLOR_NEW = (255, 255, 0) # Yellow for the point currently being scanned

# Initialize Pygame
pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Arduino ultrasonic radar")
clock = pygame.time.Clock()
font = pygame.font.SysFont("monospace", 18, bold=True)

# Initialize Serial
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    ser.flush() 
except Exception as e:
    print(f"Error: {e}")
    pygame.quit()
    sys.exit()

# Data storage: {angle: distance}
points = {} 
current_angle = 90

def draw_radar_grid():
    screen.fill(COLOR_BG)
    for i in range(1, 5):
        radius = (WIDTH // 2.2 / 4) * i
        pygame.draw.circle(screen, COLOR_GRID, CENTER, int(radius), 1)
        dist_label = font.render(f"{int(MAX_DISTANCE/4 * i)}cm", True, COLOR_GRID)
        screen.blit(dist_label, (CENTER[0] + radius + 5, CENTER[1] - 10))

    for angle in range(0, 181, 30):
        rad = math.radians(angle)
        x = CENTER[0] + (WIDTH // 2.2) * math.cos(rad)
        y = CENTER[1] - (WIDTH // 2.2) * math.sin(rad)
        pygame.draw.line(screen, COLOR_GRID, CENTER, (x, y), 1)
        ang_label = font.render(f"{angle}°", True, COLOR_GRID)
        lx = CENTER[0] + (WIDTH // 2.1) * math.cos(rad) - 10
        ly = CENTER[1] - (WIDTH // 2.1) * math.sin(rad) - 10
        screen.blit(ang_label, (lx, ly))

def run_radar():
    global current_angle
    running = True
    
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        # 1. Read Serial Data
        while ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line and "," in line:
                    parts = line.split(",")
                    if len(parts) == 2:
                        angle = int(float(parts[0]))
                        dist = float(parts[1])
                        
                        current_angle = angle
                        # Store just the float distance. No tuple, no timestamp.
                        points[angle] = min(dist, MAX_DISTANCE)
            except (ValueError, IndexError):
                continue

        # 2. Draw Graphics
        draw_radar_grid()
        
        # 3. Draw Scan Beam
        beam_rad = math.radians(current_angle)
        beam_x = CENTER[0] + (WIDTH // 2.2) * math.cos(beam_rad)
        beam_y = CENTER[1] - (WIDTH // 2.2) * math.sin(beam_rad)
        pygame.draw.line(screen, COLOR_BEAM, CENTER, (beam_x, beam_y), 3)

        # 4. Draw Objects
        for ang, d in points.items():
            if d >= MAX_DISTANCE:
                continue
            rad = math.radians(ang)
            dist_px = (d / MAX_DISTANCE) * (WIDTH // 2.2)
            x = int(CENTER[0] + dist_px * math.cos(rad))
            y = int(CENTER[1] - dist_px * math.sin(rad))
            
            # If the point is exactly where the beam is, highlight it yellow
            if abs(ang - current_angle) < 2:
                pygame.draw.circle(screen, COLOR_NEW, (x, y), 6)
            else:
                pygame.draw.circle(screen, COLOR_OBJ, (x, y), 4)

        # 5. UI Overlay
        dist_now = points.get(current_angle, 0)
        ui_text = font.render(f"ANGLE: {current_angle}° | DISTANCE: {dist_now:.1f} cm", True, COLOR_BEAM)
        screen.blit(ui_text, (30, 30))
        pygame.display.flip()
        clock.tick(60)

    ser.close()
    pygame.quit()

if __name__ == "__main__":
    run_radar()
