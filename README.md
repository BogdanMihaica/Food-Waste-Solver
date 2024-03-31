# Food-Waste-Solver
This project was made using C on Linux.
It is a prototype to an idea of a bigger project that is intended to solve the problem of restaurants wasting unconsumed food by linking charity associations (or even people) with restaurants. The restaurants upload food to the database (SQLite) then charity companies search for food on the app and then they order it using a shopping-cart feature.

(in linux terminal): Using a pseudo-login feature you log in into your account by using the command donor/receiver followed by your name.
This command logs you in if the account exists in the database or creates a new user with your name and your role (donor/receiver). 

Here is a full list of the available commands of the project:

(not signed)
donor / receiver [Name] - selects the type of client and its name

(signed)
[Donor]
list new food - starts the process of listing a new food 
remove item - will print the list of items you have published and its corresponding value. 
                you will have to type the number which corresponds to the item you want to remove. 
list my items - lists your current items  

[Receiver] , [Donor]
display [name]
show categories - displays a list of food categories 
display items from (name) - displays items from a category by its name (if it exists)   
quit
[Receiver]
add to cart (id) (number of servings) - adds the food with the specific id to the cart and the number of servings   
remove from cart (id) (number of servings) - adds the food with the specific id to the cart and the number of servings  
finish order - if there is something in the cart, for every product in the cart 
show cart - displays all items in your cart



