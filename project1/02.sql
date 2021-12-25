-- Print the nicknames of the Pokémon that are level 50 or higher
-- among the Pokémon trainer caught, in alphabetical order.

SELECT  CP.nickname
FROM    CatchedPokemon AS CP
WHERE   CP.level >= 50
ORDER BY CP.nickname ASC;
