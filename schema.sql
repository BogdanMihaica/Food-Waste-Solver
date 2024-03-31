
CREATE TABLE IF NOT EXISTS Food (
    Id INTEGER PRIMARY KEY,
    Name TEXT NOT NULL,
    Category TEXT NOT NULL,
    nameOfRestaurant TEXT NOT NULL,
    servings INTEGER NOT NULL,
    gramsPerServing INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS Donors (
    Name TEXT PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS Receivers (
    Name TEXT PRIMARY KEY
);



INSERT INTO Food (id,name,category,nameOfRestaurant, servings, gramsPerServing) values (123, 'Lasagna','Italian', 'Donor1', 20, 300);