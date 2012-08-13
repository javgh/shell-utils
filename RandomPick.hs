{-# LANGUAGE OverloadedStrings #-}
module Main where

import Control.Applicative
import Shelly
import System.Random

import qualified Data.Text.Lazy as TL

main = do
    files <- TL.lines <$> (shelly $ silently $ run "find" ["-type", "f"])
    when (length files > 0) $ do
        pick <- randomRIO (0, length files - 1)
        putStrLn $ TL.unpack (files !! pick)
