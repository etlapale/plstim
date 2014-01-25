{-# LANGUAGE ForeignFunctionInterface #-}

module PlStim (
)
where

import Foreign.Ptr


foreign export ccall hello :: IO ()

hello :: IO ()
hello = putStrLn "Hello Haskell world!"
