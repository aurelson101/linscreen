import os
import subprocess
import argparse

def is_wayland():
    return 'WAYLAND_DISPLAY' in os.environ

def capture_screen_wayland():
    try:
        region = subprocess.check_output(['slurp'], text=True).strip()
        subprocess.run(['grim', '-g', region, '-'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de la capture de l'écran : {e}")
    except Exception as e:
        print(f"Une erreur inattendue s'est produite : {e}")

def main():
    parser = argparse.ArgumentParser(description='Capture d\'écran avec linscreen.')
    parser.add_argument('--wayland', action='store_true', help='Utiliser Wayland pour la capture d\'écran.')
    args = parser.parse_args()

    if args.wayland and is_wayland():
        capture_screen_wayland()
    else:
        print("L'option --wayland n'est pas supportée ou l'environnement n'est pas Wayland.")

if __name__ == '__main__':
    main()