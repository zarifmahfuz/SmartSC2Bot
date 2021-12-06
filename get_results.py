from subprocess import check_output
import sys

def main():
    arg = sys.argv

    try:
        num_times_to_run_bot = int(arg[1])
        bot_arguements = " ".join(arg[2:])

    except IndexError:
        print("Not enough arguments")
        sys.exit()
    
    run_bot(num_times_to_run_bot, bot_arguements)

def run_bot(n, bot_arguements):
    """
    Runs the Bot n times and records the output

    Parameters:
        n: number of times the Bot needs to be run
        bot_arguements: the CLI arguements that will run the bot
    """

    num_wins = 0
    num_undecided = 0

    seconds_elapsed = 0
    ai_difficulty = None
    player_result = None
    map_name = None

    for i in range(n):
        try:
            out = check_output(bot_arguements, shell=True).decode().splitlines()
        except Exception:
            print("GAME", i + 1)
            print("--------------------------------")
            print("GAME RESULT: LOSS (GAME CRASHED OR COULD NOT BE RUN)")
            print("--------------------------------")
            print()
            continue

        seconds_elapsed = out[-1]
        ai_difficulty = out[-2]
        player_result = out[-3]
        map_name = out[-5]

        if (player_result == "WIN"):
            num_wins += 1
        elif (player_result == "UNDECIDED"):
            num_undecided += 1
        
        print("GAME", i + 1)
        print("--------------------------------")
        
        print("GAME RESULT:", player_result)
        print("GAME DURATION (SECONDS):", seconds_elapsed)
        print("--------------------------------")
        print()

    compute_results(seconds_elapsed, ai_difficulty, num_wins, map_name, num_undecided, n)


def compute_results(seconds_elapsed, ai_difficulty, num_wins, map_name, num_undecided, n):
    """
    Computes the win percent of the Bot and prints other relevant details

    Parameters:
        seconds_elapsed: number of seconds the game lasted (in-game time)
        ai_difficulty: AI difficulty the bot played against
        num_wins: number of times the Bot won against the AI
        map_name: The name of the map where the match was played
        num_undecided: number of times the game result was UNDECIDED
        n: number of times the Bot played the AI
    """

    print("-----------------------------AGGREGRATE RESULTS-----------------------------")
    print("Win Percent:", num_wins/n * 100, "%")
    print("Number of Undecided games:", num_undecided)
    print("Map:", map_name)
    print("AI Difficulty:", ai_difficulty)

if __name__ == "__main__":
    main()
