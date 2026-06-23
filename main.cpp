#include <iostream>
#include "Database.hpp"

int main() {
    // Instantiate our database
    Database db;

    std::cout << "--- Testing In-Memory DB ---\n";

    // 1. Add some data (Simulating SET commands)
    db.setInt("player_health", 100);
    db.setString("player_name", "Andrei");
    db.setString("server_status", "Active");

    std::cout << "\n--- Fetching Data ---\n";

    // 2. Retrieve the data (Simulating GET commands)
    db.get("player_name");
    db.get("player_health");

    // Try to get a key that doesn't exist
    db.get("player_score");

    std::cout << "\n--- Deleting Data ---\n";

    // 3. Delete data (Simulating DEL commands)
    db.remove("player_health");
    
    // Verify it was deleted
    db.get("player_health");

    return 0;
}