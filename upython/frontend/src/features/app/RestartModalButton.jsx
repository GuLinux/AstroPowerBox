import { useDispatch } from "react-redux"
import { restartAsync } from "./appSlice"
import { ConfirmModal } from "../../ConfirmModal"
import Button from 'react-bootstrap/Button';

const defaultButtonText = 'Restart'
const defaultModalMessage = 'Are you sure you want to restart AstroPowerBox?'

export const RestartModalButton = ({ buttonText=defaultButtonText, modalText=defaultModalMessage, ...buttonProps}) => {
    const dispatch = useDispatch()
    return <ConfirmModal
                confirmButton={buttonText}
                text={modalText}
                onConfirm={() => dispatch(restartAsync())}
                RenderButton={(props) => <Button variant='danger' {...props} {...buttonProps}>{buttonText}</Button>}
            />
}